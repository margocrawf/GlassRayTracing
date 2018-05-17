#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "mat4x4.h"
#include <math.h>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <limits>
#include <algorithm>


class Material
{
    vec3 color;
public:
    float mu = 0;
    vec3 reflectance;
    vec3 refractance;
    Material(vec3 color) : color(color) {
    }

    virtual vec3 shade(vec3 position, vec3 normal, vec3 viewDir,
                       vec3 lightDir, vec3 powerDensity) {
        return getColor(position, normal, viewDir);
    }

	virtual vec3 getColor(
		vec3 position,
		vec3 normal,
		vec3 viewDir)
	{
		return color * std::max(normal.dot(viewDir), 0.0f); //multiply by dot product
	}
};

class DiffuseMaterial: public Material
{
    vec3 kd; // diffuse light constant

public:
    DiffuseMaterial(vec3 color) : 
        Material(color) {
        kd = color;
    }

    vec3 shade(vec3 position, vec3 normal, vec3 viewDir,
               vec3 lightDir, vec3 powerDensity) {
        // L = powerDensity . k_d(n * lightDir)+
        return powerDensity * ( kd * (normal.dot(lightDir)));
    }
};

class Metal: public Material
{
    vec3 ks; // diffuse light constant
    vec3 kd; // diffuse light constant
    int gamma;
public:
    Metal(vec3 color) : 
        Material(vec3(0.2,0.2,0.2)) {
        ks = vec3(0.5,0.5,0.5);
        kd = vec3(0.2,0.2,0.2);
        gamma = 6;
        reflectance = vec3(0.9,0.9,0.9);
    }

    vec3 shade(vec3 position, vec3 normal, vec3 viewDir,
               vec3 lightDir, vec3 powerDensity) {
        // halfway = (lightDir + viewDir.normalize()
        // L = powerDensity . k_s(halfway * n)
        vec3 kd_term = (powerDensity * ( kd * (normal.dot(lightDir))));
        vec3 halfway = (lightDir + viewDir).normalize();
        vec3 ks_term = powerDensity * ( ks * pow((halfway.dot(normal)), gamma));
        return kd_term + ks_term;
    }
};


class Glass: public Material
{
    vec3 ks; // diffuse light constant
    vec3 kd; // diffuse light constant
    int gamma;
public:
    Glass(vec3 color = vec3(0.6,0.8,0.8)) : 
        Material(color) {
        mu = 1.46;
        ks = vec3(0.1,0.1,0.1);
        kd = color;
        //kd = vec3(0,0,0);
        //ks = vec3(0,0,0);
        gamma = 6;
        refractance = vec3(0.9,0.9,0.9);
        reflectance = vec3(0.1,0.1,0.1);
        //refractance = vec3(0,0,0);
        }

    vec3 shade(vec3 position, vec3 normal, vec3 viewDir,
               vec3 lightDir, vec3 powerDensity) {
        // halfway = (lightDir + viewDir.normalize()
        // L = powerDensity . k_s(halfway * n)
        vec3 kd_term = (powerDensity * ( kd * (normal.dot(lightDir))));
        vec3 halfway = (lightDir + viewDir).normalize();
        vec3 ks_term = powerDensity * ( ks * pow((halfway.dot(normal)), gamma));
        return kd_term + ks_term;
    }
};

class SpecularMaterial: public Material
{
    vec3 ks; // diffuse light constant
    vec3 kd; // diffuse light constant
    int gamma;

public:
    SpecularMaterial(vec3 color) : 
        Material(color) {
        ks = vec3(0.5,0.5,0.5);
        kd = color;
        gamma = 6;
    }

    vec3 shade(vec3 position, vec3 normal, vec3 viewDir,
               vec3 lightDir, vec3 powerDensity) {
        // halfway = (lightDir + viewDir.normalize()
        // L = powerDensity . k_s(halfway * n)
        vec3 kd_term = (powerDensity * ( kd * (normal.dot(lightDir))));
        vec3 halfway = (lightDir + viewDir).normalize();
        vec3 ks_term = powerDensity * ( ks * pow((halfway.dot(normal)), gamma));
        return kd_term + ks_term;
    }
};

class Ball : public Material
{
    vec3 ks; // diffuse light constant
    int gamma;

public:
    Ball():
        Material(vec3(1,1,1))
{
        ks = vec3(0.5,0.5,0.5);
        gamma = 6;
}

    vec3 shade(vec3 position, vec3 normal, vec3 viewDir,
               vec3 lightDir, vec3 powerDensity) {

        float phi = atan2(normal.x, normal.z);
        vec3 red = shade_color(vec3(1,0,0), position, normal, viewDir,
                lightDir, powerDensity);
        vec3 white = shade_color(vec3(1,1,1), position, normal, viewDir,
                lightDir, powerDensity);
        return std::fmod(std::fmod(phi, 3.14/8)+3.14/8,3.14/8) < 3.14/16 ? red : white;

    }

    vec3 shade_color(vec3 kd, vec3 position, vec3 normal, vec3 viewDir,
               vec3 lightDir, vec3 powerDensity) {
        // color the ball based on the kd at that point
        vec3 kd_term = (powerDensity * ( kd * (normal.dot(lightDir))));
        vec3 halfway = (lightDir + viewDir).normalize();
        vec3 ks_term = powerDensity * ( ks * pow((halfway.dot(normal)), gamma));
        return kd_term + ks_term;
    }
};

class Wood : public Material
{
	float scale;
	float turbulence;
	float period;
	float sharpness;
public:
	Wood():
		Material(vec3(1, 1, 1))
	{
		scale = 16;
		turbulence = 500;
		period = 8;
		sharpness = 10;
	}

    float snoise(vec3 r) {
        unsigned int x = 0x0625DF73;
        unsigned int y = 0xD1B84B45;
        unsigned int z = 0x152AD8D0;
        float f = 0;
        for(int i=0; i<32; i++) {
            vec3 s(	x/(float)0xffffffff,
                    y/(float)0xffffffff, 
                    z/(float)0xffffffff);
            f += sin(s.dot(r));
            x = x << 1 | x >> 31;
            y = y << 1 | y >> 31;
            z = z << 1 | z >> 31;
        }
        return f / 64.0 + 0.5;
    }

    vec3 shade(vec3 position, vec3 normal, vec3 viewDir,
                       vec3 lightDir, vec3 powerDensity) {
        return getColor(position, normal, viewDir);
        //return normal;
    }

	vec3 getColor(
		vec3 position,
		vec3 normal,
		vec3 viewDir)
	{
		//return normal;
		float w = position.x * period + pow(snoise(position * scale), sharpness)*turbulence + 10000.0;
		w -= int(w);
		return (vec3(1, 0.3, 0) * w + vec3(0.35, 0.1, 0.05) * (1-w)) * normal.dot(viewDir);
	}
};
