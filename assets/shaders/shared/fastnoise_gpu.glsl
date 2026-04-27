#ifndef FASTNOISE_GPU_GLSL
#define FASTNOISE_GPU_GLSL

// GPU implementation of the FastNoise2 2D perlin noise function

/*
MIT License

Copyright (c) 2020 Jordan Peck

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define PRIMES_X int(0xF797C5C7)
#define PRIMES_Y int(0x6C060C89)
#define PRIMES_Z int(0x465FD04F)
#define PRIMES_W int(0xF7A62279)

const float kRoot2f = float(1.4142135623730950488016887242097);

#define HASHMULTIPLIER_A int(0xB7E0A5F5)

float FMulAdd(float a, float b, float c) {
    return a * b + c;
}

float ScaleOutput( float value, float nativeMin, float nativeMax, float targetRangeMin, float targetRangeScale )
{
    return 1.0f / ( nativeMax - nativeMin ) * targetRangeScale
    * (value - nativeMin)
    + targetRangeMin;
}

void ScalePositions(inout float x, inout float y, in float frequency) {
    x *= frequency;
    y *= frequency;
}

float SelectHighBit(int mask, float ifTrue, float ifFalse) {
    return mix(ifFalse,ifTrue, (mask >> 31) & 1);
}

float InterpQuintic( float t )
{
    return t * t * t * FMulAdd( t, FMulAdd( t, float( 6 ), float( -15 )), float( 10 ) );
}

float GetGradientDotPerlin( int hash, float fX, float fY )
{
    int xBits = floatBitsToInt(fX) ^ ((hash << 31));
    int yBits = floatBitsToInt(fY) ^ ((hash >> 1) << 31);
    fX = intBitsToFloat(xBits);
    fY = intBitsToFloat(yBits);

    bool swapAxes = ((hash << 29) & 0x80000000) != 0; // bit 2 into sign pos
    // equivalently:
    // bool swapAxes = (hash & 4) != 0;
    float u = swapAxes ? fY : fX;
    float v = swapAxes ? fX : fY;

    return float( 1.0f + kRoot2f ) * u + v;
}

int HashPrimes(int seed, int x, int y) {
    int hash = seed;
    hash ^= (x ^ y);

    hash *= int(HASHMULTIPLIER_A);

    return (hash >> 15) ^ hash;
}

float GenPerlin2D( int seed, float x, float y, float frequency )
{
    //seed += int( mSeedOffset );
    ScalePositions( x, y, frequency);

    float xs = floor( x );
    float ys = floor( y );

    int x0 = int( xs ) * int( PRIMES_X );
    int y0 = int( ys ) * int( PRIMES_Y );
    int x1 = x0 + int( PRIMES_X );
    int y1 = y0 + int( PRIMES_Y );

    float xf0 = xs = x - xs;
    float yf0 = ys = y - ys;
    float xf1 = xf0 - float( 1 );
    float yf1 = yf0 - float( 1 );

    xs = InterpQuintic( xs );
    ys = InterpQuintic( ys );

    float value = mix(
        mix( GetGradientDotPerlin( HashPrimes( seed, x0, y0 ), xf0, yf0 ), GetGradientDotPerlin( HashPrimes( seed, x1, y0 ), xf1, yf0 ), xs ),
        mix( GetGradientDotPerlin( HashPrimes( seed, x0, y1 ), xf0, yf1 ), GetGradientDotPerlin( HashPrimes( seed, x1, y1 ), xf1, yf1 ), xs ), ys );

    const float kBounding = 1.726796627044677734375f;

    return ScaleOutput( value, -kBounding, kBounding, -1.0f, 2.0f );
}

#endif