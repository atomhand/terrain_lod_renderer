#pragma once
#include <glm/glm.hpp>

using glm::vec2, glm::vec3, glm::dot, glm::vec4;

void FAST32_hash_2D( vec2 gridcell, vec4& hash_0, vec4& hash_1 )	//	generates 2 random numbers for each of the 4 cell corners
{
    //    gridcell is assumed to be an integer coordinate
    const vec2 OFFSET = vec2( 26.0, 161.0 );
    const float domain = 71.0;
    const vec2 SOMELARGEFLOATS = vec2( 951.135664, 642.949883 );
    vec4 P = vec4( gridcell.x, gridcell.y, gridcell.x + 1.f, gridcell.y + 1.f);
    P = P - glm::floor(P * ( 1.0f / domain )) * domain;
    P += glm::vec4(OFFSET.x, OFFSET.y, OFFSET.x, OFFSET.y);
    P *= P;
    P = glm::vec4(P.x, P.z, P.x, P.z) * glm::vec4(P.y, P.y, P.w, P.w);
    hash_0 = glm::fract( P * ( 1.0f / SOMELARGEFLOATS.x ) );
    hash_1 = glm::fract( P * ( 1.0f / SOMELARGEFLOATS.y ) );
}

//
//	SimplexPerlin2D  ( simplex gradient noise )
//	Perlin noise over a simplex (triangular) grid
//	Return value range of -1.0->1.0
//	http://briansharpe.files.wordpress.com/2012/01/simplexperlinsample.jpg
//
//	Implementation originally based off Stefan Gustavson's and Ian McEwan's work at...
//	http://github.com/ashima/webgl-noise
//
float SimplexPerlin2D( vec2 P )
{
    //	simplex math constants
    const float SKEWFACTOR = 0.36602540378443864676372317075294;			// 0.5*(sqrt(3.0)-1.0)
    const float UNSKEWFACTOR = 0.21132486540518711774542560974902;			// (3.0-sqrt(3.0))/6.0
    const float SIMPLEX_TRI_HEIGHT = 0.70710678118654752440084436210485;	// sqrt( 0.5 )	height of simplex triangle
    const vec3 SIMPLEX_POINTS = vec3( 1.0-UNSKEWFACTOR, -UNSKEWFACTOR, 1.0-2.0*UNSKEWFACTOR );		//	vertex info for simplex triangle

    //	establish our grid cell.
    P *= SIMPLEX_TRI_HEIGHT;		// scale space so we can have an approx feature size of 1.0  ( optional )
    vec2 Pi = floor( P + dot( P, vec2( SKEWFACTOR ) ) );

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    vec4 hash_x, hash_y;
    FAST32_hash_2D( Pi, hash_x, hash_y );
    //SGPP_hash_2D( Pi, hash_x, hash_y );

    //	establish vectors to the 3 corners of our simplex triangle
    vec2 v0 = Pi - dot( Pi, vec2( UNSKEWFACTOR ) ) - P;
    vec4 v1pos_v1hash = (v0.x < v0.y) ? vec4(SIMPLEX_POINTS.x, SIMPLEX_POINTS.y, hash_x.y, hash_y.y) : vec4(SIMPLEX_POINTS.y, SIMPLEX_POINTS.x, hash_x.z, hash_y.z);
    vec4 v12 = vec4( v1pos_v1hash.x, v1pos_v1hash.y, SIMPLEX_POINTS.z, SIMPLEX_POINTS.z ) + glm::vec4(v0.x, v0.y, v0.x, v0.y);

    //	calculate the dotproduct of our 3 corner vectors with 3 random normalized vectors
    vec3 grad_x = vec3( hash_x.x, v1pos_v1hash.z, hash_x.w ) - 0.49999f;
    vec3 grad_y = vec3( hash_y.x, v1pos_v1hash.w, hash_y.w ) - 0.49999f;
    vec3 grad_results = inversesqrt( grad_x * grad_x + grad_y * grad_y ) * ( grad_x * vec3( v0.x, v12.x, v12.z ) + grad_y * vec3( v0.y, v12.y, v12.w ) );

    //	Normalization factor to scale the final result to a strict 1.0->-1.0 range
    //	x = ( sqrt( 0.5 )/sqrt( 0.75 ) ) * 0.5
    //	NF = 1.0 / ( x * ( ( 0.5 � x*x ) ^ 4 ) * 2.0 )
    //	http://briansharpe.wordpress.com/2012/01/13/simplex-noise/#comment-36
    const float FINAL_NORMALIZATION = 99.204334582718712976990005025589;

    //	evaluate the surflet, sum and return
    vec3 m = vec3( v0.x, v12.x, v12.z ) * vec3( v0.x, v12.x, v12.z ) + vec3( v0.y, v12.y, v12.w ) * vec3( v0.y, v12.y, v12.w );
    m = glm::max(0.5f - m, 0.0f);		//	The 0.5 here is SIMPLEX_TRI_HEIGHT^2
    m = m*m;
    return dot(m*m, grad_results) * FINAL_NORMALIZATION;
}