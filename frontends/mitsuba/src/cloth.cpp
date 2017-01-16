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

//include <mitsuba/render/scene.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/hw/basicshader.h>
#include <mitsuba/core/warp.h>
//#include <mitsuba/render/texture.h>
#include <mitsuba/core/fresolver.h>
//#include <mitsuba/core/qmc.h>

#define TL_THUNDERLOOM_IMPLEMENTATION
#include "../../../src/thunderloom.h"

//TODO: Implement these in mitsuba
//Stubbed texture eval functions for now.
float tl_eval_texmap_mono(void *texmap, void *context) {
    return 0.f;
}
tlColor tl_eval_texmap_color(void *texmap, void *context) {
    tlColor col = {1.f, 0.f, 0.f};
    return col;
}
float tl_eval_texmap_mono_lookup(void *texmap, float u, float v, void *context) {
    return 0.f;
}

    MTS_NAMESPACE_BEGIN

    //TODO(Vidar): Write documentation
    class Cloth : public BSDF {
        public:
            Cloth(const Properties &props)
                : BSDF(props) {
                    // Reflectance is used to modify the color of the cloth
                    /* For better compatibility with other models, support both
                       'reflectance' and 'diffuseReflectance' as parameter names */
                    /*m_reflectance = new ConstantSpectrumTexture(props.getSpectrum(
                        props.hasProperty("reflectance") ? "reflectance"
                            : "diffuseReflectance", Spectrum(.5f)));*/

                    //Set main paramaters
                    m_weave_params->uscale = props.getFloat("uscale", 1.f);
                    m_weave_params->vscale = props.getFloat("vscale", 1.f);
                    m_weave_params->realworld_uv = 0;
                    
#ifdef USE_WIFFILE
                    // LOAD WIF FILE
                    std::string wiffilename =
                        Thread::getThread()->getFileResolver()->
                        resolve(props.getString("wiffile")).string();

                    const char *errors;
                    m_weave_params = tl_weave_pattern_from_file(
                            wiffilename.c_str(), &errors);
                    if(!m_weave_params){
                        printf("ERROR! %s\n", errors);
                    }
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
                    tlWeavePatternFromData(m_weave_params, warp_above,
                            warp_color, weft_color, 3, 3);
#endif

                    //Default yarn paramters
                    tlYarnType *yrn0 = &m_weave_params->yarn_types[0];
                    yrn0->umax = props.getFloat("yrn0_bend", 0.5);
                    yrn0->psi = props.getFloat("yrn0_psi", 0.5);
                    yrn0->alpha = props.getFloat("yrn0_alpha", 0.5);
                    yrn0->beta = props.getFloat("yrn0_beta", 0.5);
                    yrn0->delta_x = props.getFloat("yrn0_delta_x", 0.5);
                    yrn0->specular_strength = 
                        props.getFloat("yrn0_specular_strength", 0.1f);
                    yrn0->specular_noise = 
                        props.getFloat("yrn0_specular_noise", 0.f);
                    tl_prepare(m_weave_params);
        }

        Cloth(Stream *stream, InstanceManager *manager)
            : BSDF(stream, manager) {
                //TODO(Vidar):Read parameters from stream
                //m_reflectance = static_cast<Texture *>(manager->getInstance(stream));

                configure();
            }
        ~Cloth() {
            tl_free_weave_parameters(m_weave_params);
        }

        void configure() {
            /* Verify the input parameter and fix them if necessary */
            //m_reflectance = ensureEnergyConservation(m_reflectance, "reflectance", 1.0f);

            m_components.clear();
            m_components.push_back(EDiffuseReflection | EFrontSide
                    | ESpatiallyVarying);
            m_usesRayDifferentials = true;

            BSDF::configure();
        }

        Frame getPerturbedFrame(tlPatternData data, Intersection its) const
        {
			//Get the world space coordinate vectors going along the texture u&v
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
            result.s = normalize(dpdu - result.n * dot(result.n, dpdu));
            result.t = cross(result.n, result.s);

            //Flip the normal if it points in the wrong direction
            if (dot(result.n, its.geoFrame.n) < 0)
                result.n *= -1;

            return result;
        }

        Spectrum getDiffuseReflectance(const Intersection &its) const {
            tlIntersectionData intersection_data;
            intersection_data.uv_x = its.uv.x;
            intersection_data.uv_y = its.uv.y;
            tlPatternData pattern_data = tl_get_pattern_data(intersection_data,
                m_weave_params);
            int yrntype = pattern_data.yarn_type;
            Float sstrength=tl_yarn_type_get_specular_strength(
                        m_weave_params, yrntype, NULL);
            tlColor diffuse = tl_eval_diffuse(intersection_data, pattern_data,
                m_weave_params);
            Spectrum col;
            col.fromSRGB((1.f-sstrength)*diffuse.r,
                    (1.f-sstrength)*diffuse.g,
                    (1.f-sstrength)*diffuse.b);
            return col;
        }

        Spectrum eval(const BSDFSamplingRecord &bRec, EMeasure measure) const {
            if (!(bRec.typeMask & EDiffuseReflection) || measure != ESolidAngle
                    || Frame::cosTheta(bRec.wi) <= 0
                    || Frame::cosTheta(bRec.wo) <= 0)
                return Spectrum(0.0f);

            tlIntersectionData intersection_data;
            intersection_data.uv_x = bRec.its.uv.x;
            intersection_data.uv_y = bRec.its.uv.y;
            intersection_data.wi_x = bRec.wi.x;
            intersection_data.wi_y = bRec.wi.y;
            intersection_data.wi_z = bRec.wi.z;
            intersection_data.wo_x = bRec.wo.x;
            intersection_data.wo_y = bRec.wo.y;
            intersection_data.wo_z = bRec.wo.z;

            tlPatternData pattern_data = tl_get_pattern_data(intersection_data,
                    m_weave_params);
            //Intersection perturbed(bRec.its);
            //perturbed.shFrame = getPerturbedFrame(pattern_data, bRec.its);

            //With normal displacement
            //Vector perturbed_wo = perturbed.toLocal(bRec.its.toWorld(bRec.wo));
            //No normal displacement
            Vector perturbed_wo = bRec.wo;
            float diffuse_mask = 1.f;
            //Diffuse will be black if the perturbed direction
            //lies below the surface
            if(Frame::cosTheta(bRec.wo) * Frame::cosTheta(perturbed_wo) <= 0){
                diffuse_mask = 0.f;
            }

            int yrntype = pattern_data.yarn_type;
            Float sstrength=tl_yarn_type_get_specular_strength(
                        m_weave_params, yrntype, NULL);
            Spectrum specular(sstrength * tl_eval_specular(intersection_data,
                    pattern_data, m_weave_params));
            tlColor diffuse = tl_eval_diffuse(intersection_data, pattern_data,
                m_weave_params);
            Spectrum col;
            col.fromSRGB((1.f-sstrength)*diffuse.r,
                    (1.f-sstrength)*diffuse.g,
                    (1.f-sstrength)*diffuse.b);
            return diffuse_mask * col*(INV_PI * Frame::cosTheta(perturbed_wo)) +
                specular * Frame::cosTheta(bRec.wo);
        }

        Float pdf(const BSDFSamplingRecord &bRec, EMeasure measure) const {
            if (!(bRec.typeMask & EDiffuseReflection) || measure != ESolidAngle
                    || Frame::cosTheta(bRec.wi) <= 0
                    || Frame::cosTheta(bRec.wo) <= 0)
                return 0.0f;

            const Intersection& its = bRec.its;
            tlIntersectionData intersection_data;
            intersection_data.uv_x = its.uv.x;
            intersection_data.uv_y = its.uv.y;
            intersection_data.wi_x = bRec.wi.x;
            intersection_data.wi_y = bRec.wi.y;
            intersection_data.wi_z = bRec.wi.z;
            intersection_data.wo_x = bRec.wo.x;
            intersection_data.wo_y = bRec.wo.y;
            intersection_data.wo_z = bRec.wo.z;

            tlPatternData pattern_data = tl_get_pattern_data(intersection_data,
                    m_weave_params);

            return warp::squareToCosineHemispherePdf(bRec.wo);
            /*Intersection perturbed(its);
            perturbed.shFrame = getPerturbedFrame(pattern_data, its);

            return warp::squareToCosineHemispherePdf(perturbed.toLocal(
                        its.toWorld(bRec.wo)));*/

        }

        Spectrum sample(BSDFSamplingRecord &bRec, const Point2 &sample) const {
            if (!(bRec.typeMask & EDiffuseReflection)
                    || Frame::cosTheta(bRec.wi) <= 0) return Spectrum(0.0f);
            const Intersection& its = bRec.its;
            tlIntersectionData intersection_data;
            intersection_data.uv_x = its.uv.x;
            intersection_data.uv_y = its.uv.y;
            intersection_data.wi_x = bRec.wi.x;
            intersection_data.wi_y = bRec.wi.y;
            intersection_data.wi_z = bRec.wi.z;
            intersection_data.wo_x = bRec.wo.x;
            intersection_data.wo_y = bRec.wo.y;
            intersection_data.wo_z = bRec.wo.z;

            tlPatternData pattern_data = tl_get_pattern_data(intersection_data,
                    m_weave_params);
            //Intersection perturbed(its);
            //perturbed.shFrame = getPerturbedFrame(pattern_data, its);
            //bRec.wi = perturbed.toLocal(its.toWorld(bRec.wi));
            bRec.wo = warp::squareToCosineHemisphere(sample);
            //With normal displacement
            //Vector perturbed_wo = perturbed.toLocal(bRec.its.toWorld(bRec.wo));
            //No normal displacement
            Vector perturbed_wo = bRec.wo;

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
            
            int yrntype = pattern_data.yarn_type;
            Float sstrength=tl_yarn_type_get_specular_strength(
                        m_weave_params, yrntype, NULL);
            Spectrum specular(sstrength * tl_eval_specular(intersection_data,
                    pattern_data, m_weave_params));
            tlColor diffuse = tl_eval_diffuse(intersection_data, pattern_data,
                m_weave_params);
            Spectrum col;
            col.fromSRGB((1.f-sstrength)*diffuse.r,
                    (1.f-sstrength)*diffuse.g,
                    (1.f-sstrength)*diffuse.b);
            return diffuse_mask * col + specular;
        }

        Spectrum sample(BSDFSamplingRecord &bRec, Float &pdf, const Point2 &sample) const {
            if (!(bRec.typeMask & EDiffuseReflection) || Frame::cosTheta(bRec.wi) <= 0)
                return Spectrum(0.0f);

            const Intersection& its = bRec.its;
            tlIntersectionData intersection_data;
            intersection_data.uv_x = its.uv.x;
            intersection_data.uv_y = its.uv.y;
            intersection_data.wi_x = bRec.wi.x;
            intersection_data.wi_y = bRec.wi.y;
            intersection_data.wi_z = bRec.wi.z;
            intersection_data.wo_x = bRec.wo.x;
            intersection_data.wo_y = bRec.wo.y;
            intersection_data.wo_z = bRec.wo.z;

            tlPatternData pattern_data = tl_get_pattern_data(intersection_data,
                    m_weave_params);
            //Intersection perturbed(its);
            //perturbed.shFrame = getPerturbedFrame(pattern_data, its);
            //bRec.wi = perturbed.toLocal(its.toWorld(bRec.wi));
            bRec.wo = warp::squareToCosineHemisphere(sample);
            //With normal displacement
            //Vector perturbed_wo = perturbed.toLocal(bRec.its.toWorld(bRec.wo));
            //No normal displacement
            Vector perturbed_wo = bRec.wo;
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

            int yrntype = pattern_data.yarn_type;
            Float sstrength=tl_yarn_type_get_specular_strength(
                        m_weave_params, yrntype, NULL);
            //Log(EDebug, "SPECULARSTRENGTH \"%f\"", specular_strength);
            Spectrum specular(sstrength * tl_eval_specular(intersection_data,
                    pattern_data, m_weave_params));
            tlColor diffuse = tl_eval_diffuse(intersection_data, pattern_data,
                m_weave_params);
            Spectrum col;
            col.fromSRGB((1.f-sstrength)*diffuse.r,
                    (1.f-sstrength)*diffuse.g,
                    (1.f-sstrength)*diffuse.b);
            return diffuse_mask * col + specular;
        }

        void addChild(const std::string &name, ConfigurableObject *child) {
            BSDF::addChild(name, child);
        }

        void serialize(Stream *stream, InstanceManager *manager) const {
            BSDF::serialize(stream, manager);
            //TODO(Vidar): Serialize our parameters

            //manager->serialize(stream, m_reflectance.get());
        }

        Float getRoughness(const Intersection &its, int component) const {
            return std::numeric_limits<Float>::infinity();
        }

        /*std::string toString() const {
            //TODO(Vidar): Add our parameters here...
            std::ostringstream oss;
            oss << "Cloth[" << endl
                << "  id = \"" << getID() << "\"," << endl
                << "  reflectance = " << indent(m_reflectance->toString()) << endl
                << "]";
            return oss.str();
        }*/

        //Shader *createShader(Renderer *renderer) const;

        MTS_DECLARE_CLASS()
    private:
            //ref<Texture> m_reflectance;
            tlWeaveParameters *m_weave_params;
};


/*
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
}*/

//MTS_IMPLEMENT_CLASS(SmoothDiffuseShader, false, Shader)
MTS_IMPLEMENT_CLASS_S(Cloth, false, BSDF)
MTS_EXPORT_PLUGIN(Cloth, "ThunderLoom wovencloth shader")
MTS_NAMESPACE_END
