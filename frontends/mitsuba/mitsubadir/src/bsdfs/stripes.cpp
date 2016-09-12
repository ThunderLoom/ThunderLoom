#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/texture.h>
#include <mitsuba/hw/basicshader.h>
#include <mitsuba/core/warp.h>

MTS_NAMESPACE_BEGIN
#define STRIPE_CODE m_reflectance->eval(bRec.its)*(fmod(bRec.its.p.y*0.01f,1.0f) > 0.5 ? 1.0f : 0.0f)


class Striped : public BSDF {
public:
    Striped(const Properties &props) : BSDF(props) {
        // no properties at the moment.
        m_reflectance = new ConstantSpectrumTexture(Spectrum(.9f));
        m_reflectance_dark = new ConstantSpectrumTexture(Spectrum(.0f));
    }
	
    Striped(Stream *stream, InstanceManager *manager)
		: BSDF(stream, manager) {
		m_reflectance = static_cast<Texture *>(manager->getInstance(stream));

	}
	
    void configure() {
		/* Verify the input parameter and fix them if necessary */
		//m_reflectance = ensureEnergyConservation(m_reflectance, "reflectance", 1.0f);

		//m_components.clear(); // No effect on image
		if (m_reflectance->getMaximum().max() > 0)
			m_components.push_back(0);
			//m_usesRayDifferentials = m_reflectance->usesRayDifferentials(); // no effect on image
        //	??????????????????????????????????????????????????????????????????????

		BSDF::configure();
	}

    Spectrum eval(const BSDFSamplingRecord &bRec, EMeasure measure) const {
		if (!(bRec.typeMask & EDiffuseReflection) || measure != ESolidAngle
			|| Frame::cosTheta(bRec.wi) <= 0
			|| Frame::cosTheta(bRec.wo) <= 0)
			return Spectrum(0.0f);

		return STRIPE_CODE 
			    * (INV_PI * Frame::cosTheta(bRec.wo));
    }
	
    Float pdf(const BSDFSamplingRecord &bRec, EMeasure measure) const {
		if (!(bRec.typeMask & EDiffuseReflection) || measure != ESolidAngle
			|| Frame::cosTheta(bRec.wi) <= 0
			|| Frame::cosTheta(bRec.wo) <= 0)
			return 0.0f;

        //Return the pdf used for sampling.
        //This could be explained better...
		return warp::squareToCosineHemispherePdf(bRec.wo);
	}
	
    Spectrum sample(BSDFSamplingRecord &bRec, const Point2 &sample) const {
		if (!(bRec.typeMask & EDiffuseReflection) || Frame::cosTheta(bRec.wi) <= 0)
			return Spectrum(0.0f);

		bRec.wo = warp::squareToCosineHemisphere(sample);
		bRec.eta = 1.0f;
		bRec.sampledComponent = 0;
		bRec.sampledType = EDiffuseReflection;
		return STRIPE_CODE;
	} 
	
	Spectrum sample(BSDFSamplingRecord &bRec, Float &pdf, const Point2 &sample) const {
		if (!(bRec.typeMask & EDiffuseReflection) || Frame::cosTheta(bRec.wi) <= 0)
			return Spectrum(0.0f);

		bRec.wo = warp::squareToCosineHemisphere(sample);
		bRec.eta = 1.0f;
		bRec.sampledComponent = 0;
		bRec.sampledType = EDiffuseReflection;
		pdf = warp::squareToCosineHemispherePdf(bRec.wo);
		return STRIPE_CODE;
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
    
	std::string toString() const {
		std::ostringstream oss;
		oss << "SmoothDiffuse[" << endl
			<< "  id = \"" << getID() << "\"," << endl
			<< "  reflectance = " << indent(m_reflectance->toString()) << endl
			<< "]";
		return oss.str();
	}
	
    Shader *createShader(Renderer *renderer) const;

	MTS_DECLARE_CLASS()
private:
	ref<Texture> m_reflectance;
	ref<Texture> m_reflectance_dark;
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

Shader *Striped::createShader(Renderer *renderer) const {
	return new SmoothDiffuseShader(renderer, m_reflectance.get());
}

MTS_IMPLEMENT_CLASS(SmoothDiffuseShader, false, Shader)
MTS_IMPLEMENT_CLASS_S(Striped, false, BSDF)
MTS_EXPORT_PLUGIN(Striped, "Striped diffuse BRDF")
MTS_NAMESPACE_END
