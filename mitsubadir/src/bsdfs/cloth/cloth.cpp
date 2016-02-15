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

#include <mitsuba/render/scene.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/texture.h>
#include <mitsuba/hw/basicshader.h>
#include <mitsuba/core/warp.h>
#include <mitsuba/core/fresolver.h>
#include "wif/wif.c"
#include "wif/ini.c" //TODO Snygga till! (använda scons?!)

    MTS_NAMESPACE_BEGIN

    /*!\plugin{diffuse_copy}{Smooth diffuse material}
     * \order{1}
     * \icon{bsdf_diffuse}
     * \parameters{
     *     \parameter{reflectance}{\Spectrum\Or\Texture}{
     *       Specifies the diffuse albedo of the
     *       material \default{0.5}
         *     }
         * }
         *
         * \renderings{
         *     \rendering{Homogeneous reflectance, see \lstref{diffuse-uniform}}
         *         {bsdf_diffuse_plain}
         *     \rendering{Textured reflectance, see \lstref{diffuse-textured}}
         *         {bsdf_diffuse_textured}
         * }
         *
         * The smooth diffuse material (also referred to as ``Lambertian'')
         * represents an ideally diffuse material with a user-specified amount of
         * reflectance. Any received illumination is scattered so that the surface
         * looks the same independently of the direction of observation.
         *
         * Apart from a  homogeneous reflectance value, the plugin can also accept
         * a nested or referenced texture map to be used as the source of reflectance
         * information, which is then mapped onto the shape based on its UV
         * parameterization. When no parameters are specified, the model uses the default
         * of 50% reflectance.
         *
         * Note that this material is one-sided---that is, observed from the
         * back side, it will be completely black. If this is undesirable,
         * consider using the \pluginref{twosided} BRDF adapter plugin.
         * \vspace{4mm}
         *
         * \begin{xml}[caption={A diffuse material, whose reflectance is specified
         *     as an sRGB color}, label=lst:diffuse-uniform]
         * <bsdf type="diffuse">
         *     <srgb name="reflectance" value="#6d7185"/>
         * </bsdf>
         * \end{xml}
         *
         * \begin{xml}[caption=A diffuse material with a texture map,
         *     label=lst:diffuse-textured]
         * <bsdf type="diffuse">
         *     <texture type="bitmap" name="reflectance">
         *         <string name="filename" value="wood.jpg"/>
         *     </texture>
         * </bsdf>
         * \end{xml}
         */
        class Cloth : public BSDF {
        public:
	        PaletteEntry * m_pattern_entry;
	        uint32_t m_pattern_height;
	        uint32_t m_pattern_width;
            float m_umax;
            float m_uscale;
            float m_vscale;
            Cloth(const Properties &props)
                : BSDF(props) {
                /* For better compatibility with other models, support both
                   'reflectance' and 'diffuseReflectance' as parameter names */
            m_reflectance = new ConstantSpectrumTexture(props.getSpectrum(
                props.hasProperty("reflectance") ? "reflectance"
                    : "diffuseReflectance", Spectrum(.5f)));
            m_umax = props.getFloat("umax", 0.7f);
            m_uscale = props.getFloat("uscale", 10.0f);
            m_vscale = props.getFloat("vscale", 10.0f);
       
            // LOAD WIF FILE
            std::string wiffilename = Thread::getThread()->getFileResolver()->resolve(props.getString("wiffile")).string();
            const char *filename = wiffilename.c_str();
            WeaveData *data = wif_read(filename);

            uint32_t x,y,w,h;
            m_pattern_entry = wif_get_pattern(data,&w,&h);
            m_pattern_height = h;
            m_pattern_width = w;

            printf("\nPattern:\n");
        for(y = 0; y < h; y++){
            for(x = 0; x < w; x++){
                PaletteEntry entry = m_pattern_entry[x+y*w];
                float col = entry.color[0];
                printf("%c", col > 0.5f ? 'X' : ' ');
                //printf("%c", entry.warp_above ? ' ' : 'X');
            }
            printf("\n");
        }

        wif_free_weavedata(data);

	}

	Cloth(Stream *stream, InstanceManager *manager)
		: BSDF(stream, manager) {
		m_reflectance = static_cast<Texture *>(manager->getInstance(stream));

		configure();
	}
    ~Cloth() {
        wif_free_pattern(m_pattern_entry);
    }

	void configure() {
		/* Verify the input parameter and fix them if necessary */
		m_reflectance = ensureEnergyConservation(m_reflectance, "reflectance", 1.0f);

		m_components.clear();
		if (m_reflectance->getMaximum().max() > 0)
			m_components.push_back(EDiffuseReflection | EFrontSide
				| (m_reflectance->isConstant() ? 0 : ESpatiallyVarying));
			m_usesRayDifferentials = m_reflectance->usesRayDifferentials();

		BSDF::configure();
	}

	Spectrum getDiffuseReflectance(const Intersection &its) const {
		return m_reflectance->eval(its);
	}

    struct PatternData
    {
        Spectrum color;
        Vector normal;
        Frame frame;
    };

    PatternData getPatternColor(const BSDFSamplingRecord &bRec) const {

        PatternData ret_data = {};

        float u = fmod(bRec.its.uv.x*m_uscale,1.f);
        float v = fmod(bRec.its.uv.y*m_vscale,1.f);

        if (u < 0.f) {
            u = u - floor(u);
        }
        if (v < 0.f) {
            v = v - floor(v);
        }
        
        uint32_t pattern_x = (uint32_t)(u*(float)(m_pattern_width));
        uint32_t pattern_y = (uint32_t)(v*(float)(m_pattern_height));

        AssertEx(pattern_x < m_pattern_width, "pattern_x larger than pwidth");
        AssertEx(pattern_y < m_pattern_height, "pattern_y larger than pheight");

        PaletteEntry current_point = m_pattern_entry[pattern_x + pattern_y*m_pattern_width];        

        uint32_t steps_left_warp = 0, steps_right_warp = 0;
        uint32_t steps_left_weft = 0, steps_right_weft = 0;

        if (current_point.warp_above) {
            uint32_t current_x = pattern_x;
            do{
                current_x++;
                if(current_x == m_pattern_width){
                    current_x = 0;
                }
                if(!m_pattern_entry[current_x + pattern_y*m_pattern_width].warp_above){
                    break;
                }
                steps_right_warp++;
            } while(current_x != pattern_x);

            current_x = pattern_x;
            do{
                if(current_x == 0){
                    current_x = m_pattern_width;
                }
                current_x--;
                if(!m_pattern_entry[current_x + pattern_y*m_pattern_width].warp_above){
                    break;
                }
                steps_left_warp++;
            } while(current_x != pattern_x);

        } else {
            uint32_t current_y = pattern_y;
            do{
                current_y++;
                if(current_y == m_pattern_height){
                    current_y = 0;
                }
                if(m_pattern_entry[pattern_x + current_y*m_pattern_width].warp_above){
                    break;
                }
                steps_right_weft++;
            } while(current_y != pattern_y);

            current_y = pattern_y;
            do{
                if(current_y == 0){
                    current_y = m_pattern_height;
                }
                current_y--;
                if(m_pattern_entry[pattern_x + current_y*m_pattern_width].warp_above){
                    break;
                }
                steps_left_weft++;
            } while(current_y != pattern_y);
        }

        float thread_u, thread_v;
        {
            //TODO(Vidar):Fix divisions

            float w = (steps_left_warp + steps_right_warp + 1.f);
            float x = ((u*(float)(m_pattern_width) - (float)pattern_x)
                    + steps_left_warp)/w;

            float h = (steps_left_weft + steps_right_weft + 1.f);
            float y = ((v*(float)(m_pattern_height) - (float)pattern_y)
                    + steps_left_weft)/h;

            //Rescale x and y to [-1,1]
            x = x*2.f - 1.f;
            y = y*2.f - 1.f;

            //Switch X and Y for weft, so that we always have the cylinder
            // going along the x axis
            if(!current_point.warp_above){
                float tmp = x;
                x = y;
                y = tmp;
            }

            //Calculate the u and v coordinates along the curved cylinder
            thread_u = asinf(x*sinf(m_umax));
            thread_v = asinf(y);

        }

        //Calculate the normal in thread-local coordinates
        float normal[3] = {sinf(thread_u), sinf(thread_v)*cosf(thread_u),
            cosf(thread_v)*cosf(thread_u)};

        //Get the world space coordinate vectors going along the texture u&v
        //axes
        //Vector dpdu = normalize(bRec.its.dpdu);
        //Vector dpdv = normalize(bRec.its.dpdv);
        //Calculate the world space normal
        //Vector world_n = cross(dpdv,dpdu);

        //Switch the x & y again to get back to uv space
        if(!current_point.warp_above){
            float tmp = normal[0];
            normal[0] = normal[1];
            normal[1] = tmp;
        }

        //ret_data.normal = normal[0] * dpdu + normal[1] *dpdv + normal[2]*world_n;

        ret_data.color.fromSRGB(current_point.color[0], current_point.color[1], current_point.color[2]);
        

		Float dDispDu = normal[1];
		Float dDispDv = normal[0];

		/* Build a perturbed frame -- ignores the usually
		   negligible normal derivative term */
		Vector dpdu = bRec.its.dpdu + bRec.its.shFrame.n * (
				dDispDu - dot(bRec.its.shFrame.n, bRec.its.dpdu));
		Vector dpdv = bRec.its.dpdv + bRec.its.shFrame.n * (
				dDispDv - dot(bRec.its.shFrame.n, bRec.its.dpdv));

        //set frame
		Frame result;
		result.n = normalize(cross(dpdu, dpdv));
		//result.n = normalize(ret_data.normal);
		result.s = normalize(dpdu - result.n
			* dot(result.n, dpdu));
		result.t = cross(result.n, result.s);

		if (dot(result.n, bRec.its.geoFrame.n) < 0)
			result.n *= -1;
    
        ret_data.frame = result;
        

        // TODO:
        // instead of getting color:
        // get if warp or weft is viewable at current position.
        // Want to construct our "therad reactangle".
        //      For this we need to know how long the current visible thread is.
        //          If warp
        //              walk along x in both directions on the pattern and return # of steps to the left and right before we encounter a weft.
        //          If weft
        //              -- // --
        //          Use step data to find location in rectangle.
        //              new uv coordinates
        //              ??? calculate normal in original uv coordinates. ???
        //              Somehow get normal in xyz, shading space.
        //  Then do simple test.

        return ret_data;
    }

	Spectrum eval(const BSDFSamplingRecord &bRec, EMeasure measure) const {
		if (!(bRec.typeMask & EDiffuseReflection) || measure != ESolidAngle
			|| Frame::cosTheta(bRec.wi) <= 0
			|| Frame::cosTheta(bRec.wo) <= 0)
			return Spectrum(0.0f);

		/*return m_reflectance->eval(bRec.its)
			* (INV_PI * Frame::cosTheta(bRec.wo));*/

        // Perturb the sampling record, in turn normal to match our thread normals
        PatternData pattern_data = getPatternColor(bRec);
		const Intersection& its = bRec.its;
		Intersection perturbed(its);
		perturbed.shFrame = pattern_data.frame;

		BSDFSamplingRecord perturbedQuery(perturbed,
			perturbed.toLocal(its.toWorld(bRec.wi)),
			perturbed.toLocal(its.toWorld(bRec.wo)), bRec.mode);
		if (Frame::cosTheta(bRec.wo) * Frame::cosTheta(perturbedQuery.wo) <= 0)
			return Spectrum(0.0f);
		perturbedQuery.sampler = bRec.sampler;
		perturbedQuery.typeMask = bRec.typeMask;
		perturbedQuery.component = bRec.component;
        return pattern_data.color * (INV_PI * Frame::cosTheta(perturbedQuery.wo));
	}

	Float pdf(const BSDFSamplingRecord &bRec, EMeasure measure) const {
		if (!(bRec.typeMask & EDiffuseReflection) || measure != ESolidAngle
			|| Frame::cosTheta(bRec.wi) <= 0
			|| Frame::cosTheta(bRec.wo) <= 0)
			return 0.0f;

		return warp::squareToCosineHemispherePdf(bRec.wo);
	}

	Spectrum sample(BSDFSamplingRecord &bRec, const Point2 &sample) const {
		if (!(bRec.typeMask & EDiffuseReflection) || Frame::cosTheta(bRec.wi) <= 0)
			return Spectrum(0.0f);
        
        /* OLD
        // Perturb the sampling record, in turn normal to match our thread normals
        PatternData pattern_data = getPatternColor(bRec);
		const Intersection& its = bRec.its;
		Intersection perturbed(its);
		perturbed.shFrame = pattern_data.frame;

		BSDFSamplingRecord perturbedQuery(perturbed,
			perturbed.toLocal(its.toWorld(bRec.wi)),
			perturbed.toLocal(its.toWorld(bRec.wo)), bRec.mode);
		if (Frame::cosTheta(bRec.wo) * Frame::cosTheta(perturbedQuery.wo) <= 0)
			return Spectrum(0.0f);
		perturbedQuery.sampler = bRec.sampler;
		perturbedQuery.typeMask = bRec.typeMask;
		perturbedQuery.component = bRec.component;
        //return pattern_data.color * (INV_PI * Frame::cosTheta(perturbedQuery.wo));
        */

		const Intersection& its = bRec.its;
		Intersection perturbed(its);
        PatternData pattern_data = getPatternColor(bRec);
		perturbed.shFrame = pattern_data.frame;

		BSDFSamplingRecord perturbedQuery(perturbed, bRec.sampler, bRec.mode);
		perturbedQuery.wi = perturbed.toLocal(its.toWorld(bRec.wi));
		perturbedQuery.sampler = bRec.sampler;
		perturbedQuery.typeMask = bRec.typeMask;
		perturbedQuery.component = bRec.component;
		
        //Do our usual sampling (not perturb specific)
		perturbedQuery.wo = warp::squareToCosineHemisphere(sample);
		perturbedQuery.eta = 1.0f;
		perturbedQuery.sampledComponent = 0;
		perturbedQuery.sampledType = EDiffuseReflection;
        Spectrum result = pattern_data.color;
        
        if (!result.isZero()) {
			bRec.sampledComponent = perturbedQuery.sampledComponent;
			bRec.sampledType = perturbedQuery.sampledType;
			bRec.wo = its.toLocal(perturbed.toWorld(perturbedQuery.wo));
			bRec.eta = perturbedQuery.eta;
			if (Frame::cosTheta(bRec.wo) * Frame::cosTheta(perturbedQuery.wo) <= 0)
				return Spectrum(0.0f);
		}
		return result;
	}

	Spectrum sample(BSDFSamplingRecord &bRec, Float &pdf, const Point2 &sample) const {
		/*if (!(bRec.typeMask & EDiffuseReflection) || Frame::cosTheta(bRec.wi) <= 0)
			return Spectrum(0.0f);

		bRec.wo = warp::squareToCosineHemisphere(sample);
		bRec.eta = 1.0f;
		bRec.sampledComponent = 0;
		bRec.sampledType = EDiffuseReflection;
		pdf = warp::squareToCosineHemispherePdf(bRec.wo);
        PatternData pattern_data = getPatternColor(bRec);
        return pattern_data.color;

        */


		if (!(bRec.typeMask & EDiffuseReflection) || Frame::cosTheta(bRec.wi) <= 0)
			return Spectrum(0.0f);
        
       	const Intersection& its = bRec.its;
		Intersection perturbed(its);
        PatternData pattern_data = getPatternColor(bRec);
		perturbed.shFrame = pattern_data.frame;

		BSDFSamplingRecord perturbedQuery(perturbed, bRec.sampler, bRec.mode);
		perturbedQuery.wi = perturbed.toLocal(its.toWorld(bRec.wi));
		perturbedQuery.sampler = bRec.sampler;
		perturbedQuery.typeMask = bRec.typeMask;
		perturbedQuery.component = bRec.component;
		
        //Do our usual sampling (not perturb specific)
		perturbedQuery.wo = warp::squareToCosineHemisphere(sample);
		perturbedQuery.eta = 1.0f;
		perturbedQuery.sampledComponent = 0;
		perturbedQuery.sampledType = EDiffuseReflection;
		pdf = warp::squareToCosineHemispherePdf(perturbedQuery.wo);
        Spectrum result = pattern_data.color;
        
        if (!result.isZero()) {
			bRec.sampledComponent = perturbedQuery.sampledComponent;
			bRec.sampledType = perturbedQuery.sampledType;
			bRec.wo = its.toLocal(perturbed.toWorld(perturbedQuery.wo));
			bRec.eta = perturbedQuery.eta;
			if (Frame::cosTheta(bRec.wo) * Frame::cosTheta(perturbedQuery.wo) <= 0)
				return Spectrum(0.0f);
		}
		return result;
	}

	void addChild(const std::string &name, ConfigurableObject *child) {
		if (child->getClass()->derivesFrom(MTS_CLASS(Texture))
				&& (name == "reflectance" || name == "diffuseReflectance")) {
			m_reflectance = static_cast<Texture *>(child);
		} else {
			BSDF::addChild(name, child);
		}
	}

	void serialize(Stream *stream, InstanceManager *manager) const {
		BSDF::serialize(stream, manager);

		manager->serialize(stream, m_reflectance.get());
	}

	Float getRoughness(const Intersection &its, int component) const {
		return std::numeric_limits<Float>::infinity();
	}

	std::string toString() const {
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
