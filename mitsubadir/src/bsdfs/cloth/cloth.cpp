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
            Cloth(const Properties &props)
                : BSDF(props) {
                /* For better compatibility with other models, support both
                   'reflectance' and 'diffuseReflectance' as parameter names */
            m_reflectance = new ConstantSpectrumTexture(props.getSpectrum(
                props.hasProperty("reflectance") ? "reflectance"
                    : "diffuseReflectance", Spectrum(.5f)));
       
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

    Spectrum getPatternColor(const BSDFSamplingRecord &bRec) const {
        //....
        float u = fmod(bRec.its.uv.x,1.f);
        float v = fmod(bRec.its.uv.y,1.f);

        if (u < 0.f) {
            u = u - floor(u);
        }
        if (v < 0.f) {
            v = v - floor(v);
        }
        
        uint32_t pattern_x = (uint32_t)(u*(float)(m_pattern_width));
        uint32_t pattern_y = (uint32_t)(v*(float)(m_pattern_height));

        //printf("u: %g, v: %g, pattern_x: %d, pattern_y: %d \n", u, v, pattern_x, pattern_y);

        AssertEx(pattern_x < m_pattern_width, "pattern_x larger than pwidth");
        AssertEx(pattern_y < m_pattern_height, "pattern_y larger than pheight");

        //color from wif file
        //float * col = m_pattern_entry[pattern_x + pattern_y*m_pattern_width].color;        
        
        //color from based on if thread is warp or weft
        Spectrum color;

        PaletteEntry current_point = m_pattern_entry[pattern_x + pattern_y*m_pattern_width];        
        if (current_point.warp_above) {
            color.fromSRGB(1.0, 0.0, 0.0); 
        } else {
            color.fromSRGB(0.0, 1.0, 0.0); 
        }

        //color based on how long current thread is.
        //warp
        if (current_point.warp_above) {
            uint32_t current_x = pattern_x;
            uint32_t steps_right = 0;
            uint32_t steps_left = 0;
            do{
                current_x++;
                if(current_x == m_pattern_width){
                    current_x = 0;
                }
                if(!m_pattern_entry[current_x + pattern_y*m_pattern_width].warp_above){
                    break;
                }
                steps_right++;
            }
            while(current_x != pattern_x);

            current_x = pattern_x;
            do{
                if(current_x == 0){
                    current_x = m_pattern_width;
                }
                current_x--;
                if(!m_pattern_entry[current_x + pattern_y*m_pattern_width].warp_above){
                    break;
                }
                steps_left++;
                }
                while(current_x != pattern_x);

            float length = steps_left + steps_right + 1;

            color.fromSRGB(0.0, 1.0*(float)length/10.f, 0.0); 

        } else {
            uint32_t current_y = pattern_y;
            uint32_t steps_right = 0;
            uint32_t steps_left = 0;
            do{
                current_y++;
                if(current_y == m_pattern_height){
                    current_y = 0;
                }
                if(m_pattern_entry[pattern_x + current_y*m_pattern_width].warp_above){
                    break;
                }
                steps_right++;
            }
            while(current_y != pattern_y);

            current_y = pattern_y;
            do{
                if(current_y == 0){
                    current_y = m_pattern_height;
                }
                current_y--;
                if(m_pattern_entry[pattern_x + current_y*m_pattern_width].warp_above){
                    break;
                }
                steps_left++;
                }
                while(current_y != pattern_y);

            float length = steps_left + steps_right + 1;

            color.fromSRGB(1.0*(float)length/10.f, 0.0, 0.0); 
        
        }


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

        return color;
    }

	Spectrum eval(const BSDFSamplingRecord &bRec, EMeasure measure) const {
		if (!(bRec.typeMask & EDiffuseReflection) || measure != ESolidAngle
			|| Frame::cosTheta(bRec.wi) <= 0
			|| Frame::cosTheta(bRec.wo) <= 0)
			return Spectrum(0.0f);

		/*return m_reflectance->eval(bRec.its)
			* (INV_PI * Frame::cosTheta(bRec.wo));*/
        return getPatternColor(bRec) * (INV_PI * Frame::cosTheta(bRec.wo));
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

		bRec.wo = warp::squareToCosineHemisphere(sample);
		bRec.eta = 1.0f;
		bRec.sampledComponent = 0;
		bRec.sampledType = EDiffuseReflection;
        return getPatternColor(bRec);
		//return m_reflectance->eval(bRec.its);
	}

	Spectrum sample(BSDFSamplingRecord &bRec, Float &pdf, const Point2 &sample) const {
		if (!(bRec.typeMask & EDiffuseReflection) || Frame::cosTheta(bRec.wi) <= 0)
			return Spectrum(0.0f);

		bRec.wo = warp::squareToCosineHemisphere(sample);
		bRec.eta = 1.0f;
		bRec.sampledComponent = 0;
		bRec.sampledType = EDiffuseReflection;
		pdf = warp::squareToCosineHemispherePdf(bRec.wo);
		//return m_reflectance->eval(bRec.its);
        return getPatternColor(bRec);
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
