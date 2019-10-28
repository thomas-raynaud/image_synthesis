
#include <cmath>
#include <random>

#include "vec.h"
#include "image.h"
#include "image_hdr.h"


// a remplacer par votre generation de directions
Vector direction( const float u1, const float u2 )
{
    // generer une direction uniforme sur l'hemisphere
    float phi = 2 * float(M_PI) * u1;
    
    return Vector(cos(phi) * sqrt(1 - u2), sin(phi) * sqrt(1 - u2), sqrt(u2));
}

// a remplacer par la densite supposee de la fonction de generation de directions
float pdf( const Vector& v )
{
    //return 1.0 / 2.0 * float(M_PI);
    return v.z / float(M_PI); // cos theta = v.z
}


Image density;

void sample( const int N )
{
    std::random_device seed;
    std::mt19937 rng(seed());
    std::uniform_real_distribution<float> u01(0.f, 1.f);
    
    for(int i= 0; i < N; i++)
    {
        Vector v= direction(u01(rng), u01(rng));
        
        float cos_theta= v.z;
        float phi= std::atan2(v.y, v.x) + float(M_PI);
        
        int x= phi / float(2 * M_PI) * density.width();
        int y= cos_theta * density.height();
        density(x, y)= density(x, y) + Color(1.f / float(N));
    }
}

Color eval( const Vector& v )
{
    float cos_theta= v.z;
    float phi= std::atan2(v.y, v.x) + float(M_PI);
    
    int x= phi / float(2 * M_PI) * density.width();
    int y= cos_theta * density.height();
    return density(x, y);
}

void plot( const char *filename, const Vector& wo= Vector(0, 1, 0) )
{
    const int image_size= 512;
    Image image(image_size, image_size);
    
    // camera Y up
    Vector Z= normalize(Vector(-1, 1, 1));
    Vector X= normalize(Vector(1, 1, 0));
    X= normalize(X - Z*dot(X, Z));
    Vector Y= - cross(X, Z);
    
    // loop over pixels
    for(int j= 0 ; j < image.height(); ++j)
    for(int i= 0 ; i < image.width() ; ++i)
    {
        // intersection point on the sphere
        float x = -1.1f + 2.2f * (i+.5f) / (float) image.width();
        float y = -1.1f + 2.2f * (j+.5f) / (float) image.height();
        if(x*x + y*y > 1.0f)
            continue;
        
        float z= std::sqrt(1.0f - x*x - y*y);
        Vector v= normalize(x*X + y*Y + z*Z);
        if(v.z < 0)
            continue;
        
        // evaluate function
        Color value= eval(v) / pdf(v);
        image(i, image.height() -1 - j)= Color(value, 1);
    }
    
    write_image_hdr(image, filename);
}
    

int main( )
{
    density= Image(512, 512);
    
    sample(1024*1024*16);
    plot("sphere.hdr");
    write_image_hdr(density, "density.hdr");
    
    return 0;
}
