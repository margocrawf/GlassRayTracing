

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cfloat>
#include <limits>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>		 
#include <GL/freeglut.h>	
#endif

#include <vector>

#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "mat4x4.h"
#include "shader.h"
#include "materials.h"
#include "quadricmats.h"
#include "revolutionQ.h"


const unsigned int windowWidth = 512, windowHeight = 512;

int majorVersion = 3, minorVersion = 0;

void getErrorInfo(unsigned int handle) 
{
	int logLen;
	glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 0) 
	{
		char * log = new char[logLen];
		int written;
		glGetShaderInfoLog(handle, logLen, &written, log);
		printf("Shader log:\n%s", log);
		delete log;
	}
}

void checkShader(unsigned int shader, char * message) 
{
	int OK;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &OK);
	if (!OK) 
	{
		printf("%s!\n", message);
		getErrorInfo(shader);
	}
}

void checkLinking(unsigned int program) 
{
	int OK;
	glGetProgramiv(program, GL_LINK_STATUS, &OK);
	if (!OK) 
	{
		printf("Failed to link shader program!\n");
		getErrorInfo(program);
	}
}

Shader *shader = 0;

class LightSource
{
public:
    virtual vec3 getPowerDensityAt ( vec3 x )=0;
    virtual vec3 getLightDirAt     ( vec3 x )=0;
    virtual float  getDistanceFrom ( vec3 x )=0;
};

class DirectionalLight : public LightSource
{

vec3 position;
vec3 lightDir;
vec3 powerDensity;

public:
    DirectionalLight(vec3 powerDensity, vec3 lightDir, vec3 position) :
        powerDensity(powerDensity), lightDir(lightDir), position(position) {

        }
    vec3 getPowerDensityAt ( vec3 x ){
        return powerDensity;
    }

    vec3 getLightDirAt( vec3 x ) {
        return lightDir;
    }

    float  getDistanceFrom ( vec3 x ) {
        return FLT_MAX;
    }

};

class PointLight : public LightSource
{

vec3 position;
vec3 lightDir;
vec3 powerDensity;

public:
    PointLight(vec3 powerDensity, vec3 lightDir, vec3 position) :
        powerDensity(powerDensity), lightDir(lightDir), position(position) {

        }
    vec3 getPowerDensityAt ( vec3 x ){
        return powerDensity * ( 1 / pow(getDistanceFrom(x),2));
    }

    vec3 getLightDirAt( vec3 x ) {
        return (x - position).normalize();
        
    }

    float  getDistanceFrom ( vec3 x ) {
        return sqrt( pow(x.x-position.x,2) + pow(x.y-position.y,2) + pow(x.z-position.z,2));
    }

};

// Camera class.

class Camera
{
	vec3 eye;		// World space camera position.
	vec3 lookAt;	// Center of window in world space.
	vec3 right;		// Vector from window center to window right-mid (in world space).
	vec3 up;		// Vector from window center to window top-mid (in world space).

public:
	Camera()
	{
		eye = vec3(0, 0, 2);
		lookAt = vec3(0, 0, 1);
		right = vec3(1, 0, 0);
		up = vec3(0, 1, 0);
	}
	vec3 getEye()
	{
		return eye;
	}

	// Compute ray through pixel at normalized device coordinates.

	vec3 rayDirFromNdc(float x, float y) {
		return (lookAt - eye
			+ right * x
			+ up    * y
			).normalize();
	}
};

// Ray structure.

class Ray
{
public:
    vec3 origin;
    vec3 dir;
    Ray(vec3 o, vec3 d)
    {
        origin = o;
        dir = d;
    }
};

// Hit record structure. Contains all data that describes a ray-object intersection point.

class Hit
{
public:
	Hit()
	{
		t = -1;
	}
	float t;				// Ray paramter at intersection. Negative means no valid intersection.
	vec3 position;			// Intersection coordinates.
	vec3 normal;			// Surface normal at intersection.
	Material* material;		// Material of intersected surface.
};

// Abstract base class.

class Intersectable
{
protected:
	Material* material;
public:
	Intersectable(Material* material):material(material) {}
    virtual Hit intersect(const Ray& ray)=0;
};

// Simple helper class to solve quadratic equations with the Quadratic Formula [-b +- sqrt(b^2-4ac)] / 2a, and store the results.

class QuadraticRoots
{
public:
	float t1;
	float t2;

	// Solves the quadratic a*t*t + b*t + c = 0 using the Quadratic Formula [-b +- sqrt(b^2-4ac)] / 2a, and sets members t1 and t2 to store the roots.

	QuadraticRoots(float a, float b, float c)
	{
        float discr = b * b - 4.0 * a * c;
        if ( discr < 0 ) // no roots
		{
			t1 = -1;
			t2 = -1;
			return;
		}
        float sqrt_discr = sqrt( discr );
		t1 = (-b + sqrt_discr)/2.0/a;
		t2 = (-b - sqrt_discr)/2.0/a;
	}

	// Returns the lesser of the positive solutions, or a negative value if there was no positive solution.

	float getLesserPositive()
	{
		return (0 < t1 && (t2 < 0 || t1 < t2)) ? t1 : t2;
	}

	float getGreaterPositive()
	{
		return (0 < t1 && (t2 < 0 || t1 < t2)) ? t2 : t1;
	}
};

// Object realization.

class Plane : public Intersectable
{
    vec3 normal;
    vec3 r0;
public:
    Plane(vec3 normal, vec3 r0, Material* material) : Intersectable(material), normal(normal), r0(r0) {

    }
    
    Hit intersect(const Ray& ray) {
        //float num = (r0 - ray.origin).dot(normal);
        float t = ((r0 - ray.origin).dot(normal)) / (ray.dir.dot(normal));

        Hit hit;
        hit.t = t;
        hit.material = material;
        hit.position = ray.origin + ray.dir * t;
        hit.normal = normal;

        return hit;
    }
};

class Quadric : public Intersectable
{
    mat4x4 coeffs;
public:
    Quadric(Material* material) : Intersectable(material){
        // ellipsoid: x**2 + 2y**2 + 0.5z**2 - 1 = 0
        coeffs = ellipseQ;
    }

    Quadric* setQuadric(mat4x4 quadricMat) {
        /* set the coeffs matrix, which changes the shape of the quadric
         * full list of matrices can be found in quadricsmats.h
         * some include: sphereQ, cylinderQ, coneQ, paraboloidQ, hyperboloidQ, parallelPlanesQ
         */
        coeffs = quadricMat;
        return this;
    }

    Quadric* transform(mat4x4 T) {
        coeffs =  T.invert() *  coeffs * T.invert().transpose();
        return this;
    }

    QuadraticRoots solveQuadratic(const Ray& ray) {
        // d A d**T t**2 + (d A e**T + e A d**T )t + e A e**T = 0
        vec4 d = vec4(ray.dir.x, ray.dir.y, ray.dir.z, 0);
        vec4 e = vec4(ray.origin.x, ray.origin.y, ray.origin.z, 1);

        float a = d.dot(coeffs * d);
        float b = ( d.dot( coeffs * e ) + e.dot( coeffs * d ));
        float c = e.dot( coeffs * e );

        return QuadraticRoots(a, b, c);
    }

    vec3 getNormalAt(vec3 r) {
        vec4 n = coeffs * vec4(r) + vec4(r) * coeffs;
        return vec3(n.x, n.y, n.z).normalize();
    }

    Hit intersect(const Ray& ray)
    {

		float t = solveQuadratic(ray).getLesserPositive();
			
		Hit hit;
		hit.t = t;
		hit.material = material;
		hit.position = ray.origin + ray.dir * t;
		hit.normal = getNormalAt(hit.position);

		return hit;
    }

    bool contains(vec3 r) {
        vec4 rhomo(r);
        float res = rhomo.dot( coeffs * rhomo);
        return (res < 0);
    }

};

class ClippedQuadric : public Quadric
{

public:
    Quadric* shape;
    Quadric* clipper;

    ClippedQuadric(Material* material) : Quadric(material) {
        shape = new Quadric(material);
        clipper = new Quadric(material);
    }

    ClippedQuadric* transform(mat4x4 T) {
        shape->transform(T);
        clipper->transform(T);
        return this;
    }

    ClippedQuadric* setQuadricsM(mat4x4 shapeQ, mat4x4 clipperQ) {
        shape->setQuadric(shapeQ);
        clipper->setQuadric(clipperQ);
        return this;
    }

    ClippedQuadric* setQuadricsQ(Quadric* s, Quadric* c) {
        delete shape;
        shape= s; 
        delete clipper;
        clipper = c;
        return this;
        }

    ClippedQuadric* setQuadric(mat4x4 shapeQ, float top, float bottom) {
        shape->setQuadric(shapeQ);
        clipper->setQuadric(parallelPlanesQ);
        clipper->transform(mat4x4::scaling(vec3(1, (top-bottom)/2.0, 1)) *
                           mat4x4::translation(vec3(0,bottom+(top-bottom)/2.0,0)));
    }

    Hit intersect(const Ray& ray) {
        QuadraticRoots qs = shape->solveQuadratic(ray);
        
        float loT = qs.getLesserPositive();
        float hiT = qs.getGreaterPositive();

        float t;
        if (clipper->contains(ray.origin + ray.dir*loT)) {
            // use the lower positive if you can
            t = loT;
        } else if (clipper->contains(ray.origin + ray.dir*hiT)) {
            // thats fine, just use the other one
            t = hiT;
        } else {
            // both are no good
            t = -1;
        }

        Hit hit;
        hit.t = t; 
        hit.material = material;
        hit.position = ray.origin + ray.dir * t;
        hit.normal = shape->getNormalAt(hit.position);

        return hit;
    }
};

class PinkingTree : public ClippedQuadric
{
public:

    PinkingTree(Material* material) : ClippedQuadric(material) {}

    Hit intersect(const Ray& ray) {
        QuadraticRoots qs = shape->solveQuadratic(ray);
        
        float loT = qs.getLesserPositive();
        float hiT = qs.getGreaterPositive();

        float t;

        if (clipper->contains(ray.origin + ray.dir*loT)) {
            // use the lower positive if you can
            vec3 position = ray.origin + ray.dir * loT;
            vec3 normal = shape->getNormalAt(position);
            float phi = atan2(normal.x, normal.z);
            if (std::fmod(std::fmod(phi, 3.14/8)+3.14/8,3.14/8) < 3.14/16) {
                t = loT;
            }
        } 
        if (clipper->contains(ray.origin + ray.dir*hiT)) {
            // thats fine, just use the other one
            vec3 position = ray.origin + ray.dir * hiT;
            vec3 normal = shape->getNormalAt(position);
            float phi = atan2(normal.x, normal.z);
            if (std::fmod(std::fmod(phi, 3.14/8)+3.14/8,3.14/8) < 3.14/16) {
                t = hiT;
            }
        } else {
            // both are no good
            t = -1;
        }

        Hit hit;
        hit.t = t; 
        hit.material = material;
        hit.position = ray.origin + ray.dir * t;
        hit.normal = shape->getNormalAt(hit.position);
        
        float phi = atan2(hit.normal.x, hit.normal.z);


        return hit;
    }
};

class RevolutionQuadric: public Intersectable
{
    std::vector<mat4x4> sectionQs;
    std::vector<mat4x4> clips;
    std::vector<ClippedQuadric*> quadrics;
public:
    RevolutionQuadric(Material* material, std::vector<mat4x4> sectionQs, std::vector<mat4x4> clips) : 
        Intersectable(material), sectionQs(sectionQs), clips(clips) {
            for (int i = 0; i < sectionQs.size(); i++) {
                ClippedQuadric* quad = new ClippedQuadric(material);
                quad->setQuadric(sectionQs[i], i-1, i);
                quadrics.push_back(quad);
            }
    }

    RevolutionQuadric* transform(mat4x4 t) {
        for (int i = 0; i < quadrics.size(); i++) {
            quadrics[i]->transform(t);
        }
        return this;
    }

    Hit intersect(const Ray& ray) {
        Hit best;
        best.t = -1;
        Hit h;
        for (int i = 0; i < quadrics.size(); i++) {
            h = quadrics[i]->intersect(ray);
            if ((best.t == -1) or ((h.t < best.t) and (h.t != -1))) {
                best = h;
            }
        }
        return best;
    }
};

class Box : public Intersectable
{
    Quadric* slab1;
    Quadric* slab2;
    Quadric* slab3;

public:
    Box(Material* material) : Intersectable(material) {
        slab1 = new Quadric(material);
        slab2 = new Quadric(material);
        slab3 = new Quadric(material);

        slab1->setQuadric(parallelPlanesQ);
        slab2->setQuadric(parallelPlanesQ);
        slab2->transform(mat4x4::rotation(vec3(1,0,0), 3.14/2.0));
        slab3->setQuadric(parallelPlanesQ);
        slab3->transform(mat4x4::rotation(vec3(0,0,1), 3.14/2.0));
    }

    Box* transform(mat4x4 T) {
        slab1->transform(T);
        slab2->transform(T);
        slab3->transform(T);
        return this;
    }

    Hit intersect(const Ray& ray) {
        // check the slab1 sides
        QuadraticRoots qs1 = slab1->solveQuadratic(ray);
        QuadraticRoots qs2 = slab2->solveQuadratic(ray);
        QuadraticRoots qs3 = slab3->solveQuadratic(ray);
        
        float loT1 = qs1.getLesserPositive();
        float hiT1 = qs1.getGreaterPositive();
        float loT2 = qs2.getLesserPositive();
        float hiT2 = qs2.getGreaterPositive();
        float loT3 = qs3.getLesserPositive();
        float hiT3 = qs3.getGreaterPositive();

        float t;
        Hit hit;

        if ((slab2->contains(ray.origin + ray.dir * loT1)) and 
            (slab3->contains(ray.origin + ray.dir * loT1))) {
            t = loT1;
            hit.t = t; 
            hit.position = ray.origin + ray.dir * t;
            hit.normal = slab1->getNormalAt(hit.position);
        } else if ((slab1->contains(ray.origin + ray.dir * loT2)) and 
                   (slab3->contains(ray.origin + ray.dir * loT2))) {
            t = loT2;
            hit.t = t; 
            hit.position = ray.origin + ray.dir * t;
            hit.normal = slab2->getNormalAt(hit.position);
        } else if ((slab1->contains(ray.origin + ray.dir * loT3)) and 
                   (slab2->contains(ray.origin + ray.dir * loT3))) {
            t = loT3;
            hit.t = t; 
            hit.position = ray.origin + ray.dir * t;
            hit.normal = slab3->getNormalAt(hit.position);
        } else {
            // both are no good
            t = -1;
            hit.t = t; 
            hit.position = ray.origin + ray.dir * t;
            hit.normal = slab3->getNormalAt(hit.position);
        }

        hit.material = material;

        return hit;
    }

};

class Scene
{
	Camera camera;
	std::vector<Intersectable*> objects;
	std::vector<Material*> materials;
    std::vector<LightSource*> lightSources;
public:
	Scene()
	{
        lightSources.push_back(new DirectionalLight(vec3(1,1,1), 
                               vec3(0,1,1), vec3(0,1,1)));
        //lightSources.push_back(new PointLight(vec3(10,10,10), 
        //                       vec3(0,0.2,0.35), vec3(0,0.2,0.35)));

        materials.push_back(new Metal(vec3(1,0,1)));
        materials.push_back(new Wood());
        materials.push_back(new Glass());

        /*
        mat4x4 s[4] = {section_0, section_1, section_2, section_3};
        std::vector<mat4x4> shapes(s, s+4);
        mat4x4 c[4] = {clip_0, clip_1, clip_2, clip_3};
        std::vector<mat4x4> clips(c, c+4);
        RevolutionQuadric* r = new RevolutionQuadric(materials[0], shapes, clips);
        r->transform(mat4x4::scaling(vec3(1,1,1)) * mat4x4::translation(vec3(0,-4,-5)));
        objects.push_back(r);
        */

        /*
        mat4x4 s[4] = {sphereQ, sphereQ};
        std::vector<mat4x4> shapes(s, s+2);
        mat4x4 c[4] = {clip_0, clip_1};
        std::vector<mat4x4> clips(c, c+2);
        RevolutionQuadric* r = new RevolutionQuadric(materials[2], shapes, clips);
        */
        Quadric* r = new Quadric(materials[2]);
        r->setQuadric(sphereQ);
        r->transform(mat4x4::scaling(vec3(.4,.4,.4)) * mat4x4::translation(vec3(-.1,0,0.2)));
        objects.push_back(r);

        /*
        ClippedQuadric* q = new ClippedQuadric(materials[0]);
        q->setQuadric(section_0, 0.0, 2.0);
        q->transform(mat4x4::scaling(vec3(.2,.2,.2)) * mat4x4::translation(vec3(0,-3,-5)));
        objects.push_back(q);
        */

        Plane* plane = new Plane(vec3(0,1,0), vec3(1,-0.8,1), materials[0]);
        objects.push_back(plane);

        Quadric* q1 = new Quadric(materials[1]);
        q1->setQuadric(sphereQ);
        q1->transform(mat4x4::scaling(vec3(.2,.2,.2)) * mat4x4::translation(vec3(0,0,-.3)));
        objects.push_back(q1);

	}
	~Scene()
	{
		for (std::vector<Material*>::iterator iMaterial = materials.begin(); iMaterial != materials.end(); ++iMaterial)
			delete *iMaterial;
		for (std::vector<Intersectable*>::iterator iObject = objects.begin(); iObject != objects.end(); ++iObject)
			delete *iObject;		
	}

	Camera& getCamera()
	{
		return camera;
	}

    Hit firstIntersect(const Ray& ray) {

        Hit bestHit;
        float max = std::numeric_limits<float>::max();
        bestHit.t = -1;
        for (int i = 0; i < objects.size(); i++) {
            Hit hit = objects[i]->intersect(ray);
            if (( hit.t > 0) and (hit.t < bestHit.t)) {
                bestHit = hit;
            }
            if ((bestHit.t == -1) and (hit.t > 0)) {
                bestHit = hit;
            }
        }
        return bestHit;
    }

	vec3 trace(const Ray& ray, int depth=5)
	{
        float epsilon = 0.01;
        if (depth == 0) {
            return vec3(0,0,0);
        }

        Hit hit = firstIntersect(ray);

		if(hit.t < 0)
			return vec3(0.5,0.8,0.9);
            //return vec3(0,0,0);

        vec3 color = vec3(0,0,0);
        for (int i = 0; i < lightSources.size(); i++) {
            LightSource* l = lightSources[i];
            color += hit.material->shade(hit.position, hit.normal, -ray.dir, 
                   l->getLightDirAt(hit.position), l->getPowerDensityAt(hit.position));
            vec3 shadowOrigin = hit.position + hit.normal*epsilon;
            Ray shadowRay = Ray(shadowOrigin, l->getLightDirAt(hit.position));
            Hit rayHit = firstIntersect(shadowRay);
            if (rayHit.t != -1) {
                return vec3(0,0,0);
            }
        }
        // starting vectors
        vec3 V = -ray.dir;
        vec3 N = hit.normal;
        // sign-- the product should be negative if entering the shape, 
        // positive if leaving. If entering shape, you must subtract,
        // if leaving you must add
        float sign = V.dot(N) < 0 ? -1 : 1;

        if ( ( hit.material != NULL) && 
             ( (dynamic_cast<Metal*>(hit.material ) ) or 
               ( (dynamic_cast<Glass*>(hit.material)) and (sign==-1) ) ) ) {
            // the length of the diagonal vector (parallel to normal)
            float diagonal = (2*N.dot(V));
            // the ray direction
            vec3 raydir = ((N*diagonal) - V).normalize();
            // add or subtract a very small amount, prevents repeat hits
            vec3 pos = hit.position + N*epsilon;
            // make the ray
            Ray reflRay = Ray(pos, raydir);
            // add the contribution
            color += hit.material->reflectance*trace(reflRay, depth-1);
        }

        if ((hit.material != NULL) && (dynamic_cast<Glass*>(hit.material))) {
            float mu = hit.material->mu;
            // perpendicular to normal vector
            vec3 N_perp = (N*(N.dot(V)) - V).normalize();
            // angles
            float cos_alpha = N.dot(V);
            float sin_alpha = sqrt(1 - pow(cos_alpha, 2));
            float sin_beta = sin_alpha / mu;
            float cos_beta = sqrt(1 - pow(sin_beta, 2));

            printf("%f\n", sign);

            // ray direction
            vec3 raydir = (N_perp*sin_beta + N*cos_beta).normalize();
            // add or subtract a very small amount
            vec3 pos = hit.position + N*epsilon*sign;

            //pos.print();
           
            // make the ray
            Ray refrRay = Ray(pos, raydir);
            // add the contribution
            color += hit.material->refractance*trace(refrRay, depth-1);
        }

        return color;
	}
};

Scene scene;

class FrameBuffer {
	unsigned int textureId;
	vec3 image[windowWidth * windowHeight];

public:
	FrameBuffer() {
		for(int i = 0; i < windowWidth * windowHeight; i++) image[i] = vec3(0.0, 0.0, 0.0);

		glGenTextures(1, &textureId); 
		glBindTexture(GL_TEXTURE_2D, textureId); 

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowWidth, windowHeight, 0, GL_RGB, GL_FLOAT, image);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	}

	void Bind(Shader* s)
	{
		s->UploadSamplerID();
		glBindTexture(GL_TEXTURE_2D, textureId);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowWidth, windowHeight, 0, GL_RGB, GL_FLOAT, image);
	}

	bool ComputeImage()
	{
		static unsigned int iPart = 0;

		if(iPart >= 64)
			return false;
		for(int j = iPart; j < windowHeight; j+=64)
		{
			for(int i = 0; i < windowWidth; i++)
			{
				float ndcX = (2.0 * i - windowWidth) / windowWidth;
				float ndcY = (2.0 * j - windowHeight) / windowHeight;
				Camera& camera = scene.getCamera();
				Ray ray = Ray(camera.getEye(), camera.rayDirFromNdc(ndcX, ndcY));
			
				image[j*windowWidth + i] = scene.trace(ray);
			}
		}
		iPart++;
		return true;
	}
};

class Screen {
	FrameBuffer frameBuffer;
	unsigned int vao;	

public:
	Screen() 
	{ 
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		unsigned int vbo[2];
		glGenBuffers(2, &vbo[0]);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); 
		static float vertexCoords[] = { -1, -1,		1, -1,		-1, 1,		1, 1 };

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW); 
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);   

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); 
		static float vertexTextureCoords[] = { 0, 0,	1, 0,		0, 1,		1, 1 };

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexTextureCoords), vertexTextureCoords, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);  
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL); 
	}

	void Draw(Shader* s)
	{
		if(frameBuffer.ComputeImage())
		glutPostRedisplay();

		s->Run();
		frameBuffer.Bind(s);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindVertexArray(vao); 
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisable(GL_BLEND);
	}
};

Screen *screen = 0;


void onDisplay( ) {
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	screen->Draw(shader);

    glutSwapBuffers(); 
}

void onInitialization() 
{
	glViewport(0, 0, windowWidth, windowHeight);

	shader = new Shader();
	
	screen = new Screen();
}

void onExit() 
{
	delete screen; screen = 0;
	delete shader; shader = 0;	
	printf("exit");
}

int main(int argc, char * argv[]) {
	glutInit(&argc, argv);
#if !defined(__APPLE__)
	glutInitContextVersion(majorVersion, minorVersion);
#endif
	glutInitWindowSize(windowWidth, windowHeight);				
	glutInitWindowPosition(100, 100);							
#if defined(__APPLE__)
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);  
#else
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutCreateWindow("Ray Casting");

#if !defined(__APPLE__)
	glewExperimental = true;	
	glewInit();
#endif

	printf("GL Vendor    : %s\n", glGetString(GL_VENDOR));
	printf("GL Renderer  : %s\n", glGetString(GL_RENDERER));
	printf("GL Version (string)  : %s\n", glGetString(GL_VERSION));
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	printf("GL Version (integer) : %d.%d\n", majorVersion, minorVersion);
	printf("GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	glViewport(0, 0, windowWidth, windowHeight);

	onInitialization();

	glutDisplayFunc(onDisplay);                

	glutMainLoop();
		
	onExit();
	
	return 1;
}


