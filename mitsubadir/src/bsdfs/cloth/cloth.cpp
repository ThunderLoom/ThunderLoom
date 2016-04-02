    // vim makeprg=scons
    /*
       This file is part of Mitsuba, a physically based rendering system.

       Copyright (c) 2007-2014 by Wenzel Jakob and others.

       Mitsuba is free software; you can redistribute it and/or modify
       it under the terms of the GNU General Public License Version 3
       as published by the Free Software Foundation.

       Mitsuba is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
       GNU General Public License for more details.

       You should have received a copy of the GNU General Public License
       along with this program. If not, see <http://www.gnu.org/licenses/>.
     */

    //#define DO_DEBUG
#define USE_WIFFILE

#include <mitsuba/render/scene.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/texture.h>
#include <mitsuba/hw/basicshader.h>
#include <mitsuba/core/warp.h>
#include <mitsuba/core/fresolver.h>
#include "woven_cloth/woven_cloth.cpp"

#include <mitsuba/core/qmc.h>

    MTS_NAMESPACE_BEGIN

    //TODO(Vidar): Write documentation

    /*!\plugin{cloth}{Woven Cloth}
     */
    class Cloth : public BSDF {
        public:
            Cloth(const Properties &props)
                : BSDF(props) {
                    // Reflectance is used to modify the color of the cloth
                    /* For better compatibility with other models, support both
                       'reflectance' and 'diffuseReflectance' as parameter names */
                    m_reflectance = new ConstantSpectrumTexture(props.getSpectrum(
                        props.hasProperty("reflectance") ? "reflectance"
                            : "diffuseReflectance", Spectrum(.5f)));
                    m_weave_params.uscale = props.getFloat("utiling", 1.0f);
                    m_weave_params.vscale = props.getFloat("vtiling", 1.0f);

                    //yarn properties
                    m_weave_params.umax = props.getFloat("umax", 0.7f);
                    m_weave_params.psi = props.getFloat("psi", M_PI_2);
                    
                    //fiber properties
                    m_weave_params.alpha = props.getFloat("alpha", 0.05f); //uniform scattering
                    m_weave_params.beta = props.getFloat("beta", 2.0f); //forward scattering
                    m_weave_params.delta_x = props.getFloat("deltaX", 0.5f); //deltaX for highlights (to be changed)

                    //noise
                    m_weave_params.intensity_fineness =
                        props.getFloat("intensity_fineness", 0.0f);

                    m_specular_strength = props.getFloat("specular_strength", 0.5f);

#ifdef USE_WIFFILE
                        // LOAD WIF FILE
                        std::string wiffilename =
                            Thread::getThread()->getFileResolver()->
                        resolve(props.getString("wiffile")).string();

                        wcWeavePatternFromWIF(&m_weave_params,
                                wiffilename.c_str());
#else
                    // Static pattern
                    // current: polyester pattern
                    uint8_t warp_above[] = {
                        0, 1, 1,
                        1, 0, 1,
                        1, 1, 0,
                    };
                    float warp_color[] = { 0.7f, 0.7f, 0.7f};
                    float weft_color[] = { 0.7f, 0.7f, 0.7f};
                    wcWeavePatternFromData(&m_weave_params, warp_above,
                        warp_color, weft_color, 3, 3);
#endif
        }

        Cloth(Stream *stream, InstanceManager *manager)
            : BSDF(stream, manager) {
                //TODO(Vidar):Read parameters from stream
                m_reflectance = static_cast<Texture *>(manager->getInstance(stream));

                configure();
            }
        ~Cloth() {
            wif_free_pattern(m_weave_params.pattern_entry);
        }

        void configure() {
            /* Verify the input parameter and fix them if necessary */
            m_reflectance = ensureEnergyConservation(m_reflectance, "reflectance", 1.0f);

            m_components.clear();
            m_components.push_back(EDiffuseReflection | EFrontSide
                    | ESpatiallyVarying);
            m_usesRayDifferentials = true;

            BSDF::configure();
        }

        Frame getPerturbedFrame(wcPatternData data, Intersection its) const
        {
			//Get the world space coordinate vectors going along the texture u&v
            //TODO(Peter) Is it world space though!??!
            //NOTE(Vidar) You're right, it's probably shading space... :S
            Float dDispDu = data.normal_x;
            Float dDispDv = data.normal_y;
            Vector dpdv = its.dpdv + its.shFrame.n * (
                    -dDispDv - dot(its.shFrame.n, its.dpdv));
            Vector dpdu = its.dpdu + its.shFrame.n * (
                    -dDispDu - dot(its.shFrame.n, its.dpdu));
            // dpdv & dpdu are in world space

            //set frame
            Frame result;
            result.n = normalize(cross(dpdu, dpdv));
            result.s = normalize(dpdu - result.n
                    * dot(result.n, dpdu));
            result.t = cross(result.n, result.s);

            //Flip the normal if it points in the wrong direction
            if (dot(result.n, its.geoFrame.n) < 0)
                result.n *= -1;

            return result;
        }

        Spectrum getDiffuseReflectance(const Intersection &its) const {
            wcIntersectionData intersection_data;
            intersection_data.uv_x = its.uv.x;
            intersection_data.uv_y = its.uv.y;
            wcPatternData pattern_data = wcGetPatternData(intersection_data,
                &m_weave_params);
            Spectrum col;
            col.fromSRGB(pattern_data.color_r, pattern_data.color_g,
                pattern_data.color_b);
            return col * m_reflectance->eval(its);
        }

        Spectrum eval(const BSDFSamplingRecord &bRec, EMeasure measure) const {
            if (!(bRec.typeMask & EDiffuseReflection) || measure != ESolidAngle
                    || Frame::cosTheta(bRec.wi) <= 0
                    || Frame::cosTheta(bRec.wo) <= 0)
                return Spectrum(0.0f);

            wcIntersectionData intersection_data;
            intersection_data.uv_x = bRec.its.uv.x;
            intersection_data.uv_y = bRec.its.uv.y;

            intersection_data.wi_x = bRec.wi.x;
            intersection_data.wi_y = bRec.wi.y;
            intersection_data.wi_z = bRec.wi.z;

            intersection_data.wo_x = bRec.wo.x;
            intersection_data.wo_y = bRec.wo.y;
            intersection_data.wo_z = bRec.wo.z;

            wcPatternData pattern_data = wcGetPatternData(intersection_data,
                    &m_weave_params);
            Intersection perturbed(bRec.its);
            perturbed.shFrame = getPerturbedFrame(pattern_data, bRec.its);

            Vector perturbed_wo = perturbed.toLocal(bRec.its.toWorld(bRec.wo));
            float diffuse_mask = 1.f;
            //Diffuse will be black if the perturbed direction
            //lies below the surface
            if(Frame::cosTheta(bRec.wo) * Frame::cosTheta(perturbed_wo) <= 0){
                diffuse_mask = 0.f;
            }

            Spectrum specular(m_specular_strength
                * wcEvalSpecular(intersection_data,
                    pattern_data,&m_weave_params));
            Spectrum col;
            col.fromSRGB(pattern_data.color_r, pattern_data.color_g,
                pattern_data.color_b);
            return m_reflectance->eval(bRec.its) * diffuse_mask * 
                col*(1.f - m_specular_strength) *
                (INV_PI * Frame::cosTheta(perturbed_wo)) +
                m_specular_strength*specular*Frame::cosTheta(bRec.wo);
        }

        Float pdf(const BSDFSamplingRecord &bRec, EMeasure measure) const {
            if (!(bRec.typeMask & EDiffuseReflection) || measure != ESolidAngle
                    || Frame::cosTheta(bRec.wi) <= 0
                    || Frame::cosTheta(bRec.wo) <= 0)
                return 0.0f;

            const Intersection& its = bRec.its;
            wcIntersectionData intersection_data;
            intersection_data.uv_x = its.uv.x;
            intersection_data.uv_y = its.uv.y;

            intersection_data.wi_x = bRec.wi.x;
            intersection_data.wi_y = bRec.wi.y;
            intersection_data.wi_z = bRec.wi.z;

            intersection_data.wo_x = bRec.wo.x;
            intersection_data.wo_y = bRec.wo.y;
            intersection_data.wo_z = bRec.wo.z;

            wcPatternData pattern_data = wcGetPatternData(intersection_data,
                    &m_weave_params);
            Intersection perturbed(its);
            perturbed.shFrame = getPerturbedFrame(pattern_data, its);

            return warp::squareToCosineHemispherePdf(perturbed.toLocal(
                        its.toWorld(bRec.wo)));

        }

        Spectrum sample(BSDFSamplingRecord &bRec, const Point2 &sample) const {
            if (!(bRec.typeMask & EDiffuseReflection)
                    || Frame::cosTheta(bRec.wi) <= 0) return Spectrum(0.0f);
            const Intersection& its = bRec.its;
            wcIntersectionData intersection_data;
            intersection_data.uv_x = its.uv.x;
            intersection_data.uv_y = its.uv.y;

            intersection_data.wi_x = bRec.wi.x;
            intersection_data.wi_y = bRec.wi.y;
            intersection_data.wi_z = bRec.wi.z;

            intersection_data.wo_x = bRec.wo.x;
            intersection_data.wo_y = bRec.wo.y;
            intersection_data.wo_z = bRec.wo.z;

            wcPatternData pattern_data = wcGetPatternData(intersection_data,
                    &m_weave_params);
            Intersection perturbed(its);
            perturbed.shFrame = getPerturbedFrame(pattern_data, its);

            bRec.wi = perturbed.toLocal(its.toWorld(bRec.wi));

            bRec.wo = warp::squareToCosineHemisphere(sample);
            Vector perturbed_wo = perturbed.toLocal(its.toWorld(bRec.wo));

            bRec.sampledComponent = 0;
            bRec.sampledType = EDiffuseReflection;
            bRec.eta = 1.f;
            float diffuse_mask = 0.f;
            if (Frame::cosTheta(perturbed_wo)
                    * Frame::cosTheta(bRec.wo) > 0){
                //We sample based on bRec.wo, take account of this
                diffuse_mask = Frame::cosTheta(perturbed_wo)/
                    Frame::cosTheta(bRec.wo);
            }
            Spectrum specular(m_specular_strength
                * wcEvalSpecular(intersection_data,
                    pattern_data,&m_weave_params));
            Spectrum col;
            col.fromSRGB(pattern_data.color_r, pattern_data.color_g,
                pattern_data.color_b);
            return m_reflectance->eval(bRec.its) * diffuse_mask *
                col*(1.f - m_specular_strength)
                + m_specular_strength*specular;// *
        }

        Spectrum sample(BSDFSamplingRecord &bRec, Float &pdf, const Point2 &sample) const {
            if (!(bRec.typeMask & EDiffuseReflection) || Frame::cosTheta(bRec.wi) <= 0)
                return Spectrum(0.0f);

            const Intersection& its = bRec.its;
            wcIntersectionData intersection_data;
            intersection_data.uv_x = its.uv.x;
            intersection_data.uv_y = its.uv.y;

            intersection_data.wi_x = bRec.wi.x;
            intersection_data.wi_y = bRec.wi.y;
            intersection_data.wi_z = bRec.wi.z;

            intersection_data.wo_x = bRec.wo.x;
            intersection_data.wo_y = bRec.wo.y;
            intersection_data.wo_z = bRec.wo.z;

            wcPatternData pattern_data = wcGetPatternData(intersection_data,
                    &m_weave_params);
            Intersection perturbed(its);
            perturbed.shFrame = getPerturbedFrame(pattern_data, its);
            bRec.wi = perturbed.toLocal(its.toWorld(bRec.wi));

            bRec.wo = warp::squareToCosineHemisphere(sample);
            Vector perturbed_wo = perturbed.toLocal(its.toWorld(bRec.wo));
            pdf = warp::squareToCosineHemispherePdf(bRec.wo);

            bRec.sampledComponent = 0;
            bRec.sampledType = EDiffuseReflection;
            bRec.eta = 1.f;
            float diffuse_mask = 0.f;
            if (Frame::cosTheta(perturbed_wo)
                    * Frame::cosTheta(bRec.wo) > 0){
                //We sample based on bRec.wo, take account of this
                diffuse_mask = Frame::cosTheta(perturbed_wo)/
                    Frame::cosTheta(bRec.wo);
            }
            Spectrum specular(m_specular_strength
                * wcEvalSpecular(intersection_data,
                    pattern_data,&m_weave_params));
            Spectrum col;
            col.fromSRGB(pattern_data.color_r, pattern_data.color_g,
                pattern_data.color_b);
            return m_reflectance->eval(bRec.its) * diffuse_mask *
                col*(1.f - m_specular_strength)
                + m_specular_strength*specular;// * 
        }

        void addChild(const std::string &name, ConfigurableObject *child) {
            BSDF::addChild(name, child);
        }

        void serialize(Stream *stream, InstanceManager *manager) const {
            BSDF::serialize(stream, manager);
            //TODO(Vidar): Serialize our parameters

            manager->serialize(stream, m_reflectance.get());
        }

        Float getRoughness(const Intersection &its, int component) const {
            return std::numeric_limits<Float>::infinity();
        }

        std::string toString() const {
            //TODO(Vidar): Add our parameters here...
            std::ostringstream oss;
            oss << "Cloth[" << endl
                << "  id = \"" << getID() << "\"," << endl
                << "  reflectance = " << indent(m_reflectance->toString()) << endl
                << "]";
            return oss.str();
        }

        Shader *createShader(Renderer *renderer) const;

        MTS_DECLARE_CLASS()
    private:
            ref<Texture> m_reflectance;
            float m_specular_strength;
            wcWeaveParameters m_weave_params;
};

// ================ Hardware shader implementation ================

class SmoothDiffuseShader : public Shader {
    public:
        SmoothDiffuseShader(Renderer *renderer, const Texture *reflectance)
            : Shader(renderer, EBSDFShader), m_reflectance(reflectance) {
                m_reflectanceShader = renderer->registerShaderForResource(m_reflectance.get());
            }

        bool isComplete() const {
            return m_reflectanceShader.get() != NULL;
        }

        void cleanup(Renderer *renderer) {
            renderer->unregisterShaderForResource(m_reflectance.get());
        }

        void putDependencies(std::vector<Shader *> &deps) {
            deps.push_back(m_reflectanceShader.get());
        }

        void generateCode(std::ostringstream &oss,
                const std::string &evalName,
                const std::vector<std::string> &depNames) const {
            oss << "vec3 " << evalName << "(vec2 uv, vec3 wi, vec3 wo) {" << endl
                << "    if (cosTheta(wi) < 0.0 || cosTheta(wo) < 0.0)" << endl
                << "    	return vec3(0.0);" << endl
                << "    return " << depNames[0] << "(uv) * inv_pi * cosTheta(wo);" << endl
                << "}" << endl
                << endl
                << "vec3 " << evalName << "_diffuse(vec2 uv, vec3 wi, vec3 wo) {" << endl
                << "    return " << evalName << "(uv, wi, wo);" << endl
                << "}" << endl;
        }

        MTS_DECLARE_CLASS()
    private:
            ref<const Texture> m_reflectance;
            ref<Shader> m_reflectanceShader;
};


Shader *Cloth::createShader(Renderer *renderer) const {
    return new SmoothDiffuseShader(renderer, m_reflectance.get());
}

    MTS_IMPLEMENT_CLASS(SmoothDiffuseShader, false, Shader)
MTS_IMPLEMENT_CLASS_S(Cloth, false, BSDF)
    MTS_EXPORT_PLUGIN(Cloth, "Smooth diffuse BRDF")
    MTS_NAMESPACE_END
