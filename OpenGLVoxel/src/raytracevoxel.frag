#version 330 core //This shader runs for each pixel/fragment
out vec4 FragColor;
#define MAX_NUM_TOTAL_OBJECTS 10

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct BB {
    float vertices[8];
};


uniform int numBB;
//TODO: Look into SSBO to have variable size of vertices, for now we will use a maximum value
uniform BB worldBB[MAX_NUM_TOTAL_OBJECTS  * 8];


void main()
{
    //TODO: Look into being able to only ray tracing from fragcoords with voxel boxes, for now we ray trace every pixel
    //  and check in the shader if its a voxel box or not, then color accordingly (same thing at the end of the day,
    //  just more ray tracing, most of the screen will be voxels anyway
    //FOR FUTURE SELF, the issue was that the BB vertices were slightly off than the ray traced box.. not sure what
    // was causing this but could be somethign to do with clip/world
    //Idea: Pixels are chosen based on where the box is in clip space, but getting ray direction based off of clip space,
    //and shooting those rays in world space, will not be the same direction, need to somehow convert the "pixels"
    //to world space, which might not be possible.

    //TODO: Near clipping plane and FOV are different values, we can set a larger distance to get a smaller FOV
    //  while still keeping the clipping distance small to include things that are in front of the FOV screen,
    //  pupose of that FOV plane is to create the direction vectors, so we should probably normalize them
    //  can leave them unnormalized if clip plane will further than distance, but moving camera doesn't help with this

    //TODO: Next task, set up to be able to easily pass in multiple box objects, set up shadow rays to test,
    //  then move onto shading voxels rather than the bounding boxes (should be able to use the same box object with more
    //  information) and testing boxes not being axis aligned
    //  Voxel algorithm should be similar-ish, as we can divide each box into x amount of voxels x,y,z, and use space
    // partitioning algorithsm to see which boxes the ray will potentially interact with
    Ray camRay;

    camRay.origin = cameraPos;
    vec2 resolution = vec2(1024,728);
    vec2 aspectRatio = vec2(1.40659f,1.0f);
    vec2 uv = (gl_FragCoord.xy / resolution.xy )*2.0 - 1.0;
    float u = aspectRatio.x * ( (float(gl_FragCoord.x) + 0.5f) / float(resolution.x)) - float(aspectRatio.x)/2.0f;
    float v = aspectRatio.y * ( (float(gl_FragCoord.y) + 0.5f) / float(resolution.y)) - float(aspectRatio.y)/2.0f;
    uv.x *= aspectRatio.x;
    float distance = 1.0f; //TODO: Pass this value in
    camRay.direction = normalize(distance*cameraDir + (u * cameraRight) + (v * cameraUp));


    //For the formula from the textbook txmin = (xmin - xe)/xd, we write xd as dot(raydir, right) and xe as dot(right, delta), delta being center of BB - ray origin
    //When right = aligned x axis, this is equal to saying get the x component of the origin point and direction vector, but we're now saying get x component on a rotated axis
    //Hard coding for now
    vec3 right = vec3(1,0,0);
    vec3 up = vec3(0,1,0);
    vec3 forward = vec3(0,0, 1);

    bool intersects;

    float xMax = 1;
    float yMax = 1;
    float zMax = 1;

    float xMin = 0;
    float yMin = 0;
    float zMin = 0;

    vec3 center = vec3(0.5f,0.5f,0.5f);
    vec3 delta = center - camRay.origin;

    float unalignedOriginX = dot(right, delta);
    float unalignedDirX = dot(right, camRay.direction);

    float unalignedOriginY = dot(up, delta);
    float unalignedDirY = dot(up, camRay.direction);

    float unalignedOriginZ = dot(forward, delta);
    float unalignedDirZ = dot(forward, camRay.direction);

    float a_x = 1.0f / unalignedDirX;
    float a_y = 1.0f / unalignedDirY;
    float a_z = 1.0f / unalignedDirZ;

    float t_min_x;
    float t_min_y;
    float t_min_z;

    float t_max_x;
    float t_max_y;
    float t_max_z;

    //Reasoning for these checks. If you enter any of the 3 bounds (x,y,z) and then leave any of them
    //if you are intersecting, you must have entered all 3 already, so once you leave, you can't enter any again
    //if you enter again after leaving any of the bounds, you must have been outside of the box to begin with

    vec3 nearPoint;
    vec3 farPoint;
    vec3 normal;
    float epsilon = 0.001f;
    if (a_x >= 0){
        t_min_x = a_x*(xMin - camRay.origin.x);
        t_max_x = a_x*(xMax - camRay.origin.x);
    }
    else {
        t_max_x = a_x*(xMin - camRay.origin.x);
        t_min_x = a_x*(xMax - camRay.origin.x);
    }

    if (a_y >= 0){
        t_min_y = a_y*(yMin - camRay.origin.y);
        t_max_y = a_y*(yMax - camRay.origin.y);
    }
    else {
        t_max_y = a_y*(yMin - camRay.origin.y);
        t_min_y = a_y*(yMax - camRay.origin.y);
    }

    if (a_z >= 0){
        t_min_z = a_z*(zMin - camRay.origin.z);
        t_max_z = a_z*(zMax - camRay.origin.z);
    }
    else {
        t_max_z = a_z*(zMin - camRay.origin.z);
        t_min_z = a_z*(zMax - camRay.origin.z);
    }

    float t_min = max(t_min_x,max(t_min_y, t_min_z));
    float t_max = min(t_max_x, min(t_max_y, t_max_z));

    if (t_min <= t_max){
        if (t_min > 100000){
            FragColor = vec4(0.0f, 1.0f, 1.0f, 1.0f);
            intersects = false;
            return;
        } else if (t_max < 0) {
            intersects = false;
        } else intersects = true;
    }
    else {
        intersects = false;
    }


//    if (cameraPos.x > .1f){
//        FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
//    }
//    else {
//        FragColor = vec4(1.0f,0.0f, 0.0f, 1.0f);
//    }
    if (intersects){
        //If we get here, we can check the intersection point as being t_min
        vec3 intersectionPoint = camRay.origin + t_min * camRay.direction;
        vec3 normal;
        if (abs(intersectionPoint.x - xMin) < epsilon){
            normal = vec3(-1,0,0);
        }
        else if (abs(intersectionPoint.x - xMax) < epsilon){
            normal = vec3(1,0,0);
        }
        else if (abs(intersectionPoint.y - yMin) < epsilon){
            normal = vec3(0,-1,0);
        }
        else if (abs(intersectionPoint.y - yMax) < epsilon){
            normal = vec3(0,1,0);
        }
        else if (abs(intersectionPoint.z - zMin) < epsilon){
            normal = vec3(0,0,-1);
        }
        else if (abs(intersectionPoint.z - zMax) < epsilon){
            normal = vec3(0,0,1);
        }

        //Below is a point light, if we want a directional light, the l is simply a unit vector pointing towards
        // that directional light
        vec3 lightPos = vec3(5.0f,5.0f,5.0f);
        vec3 boxAmbientCoff = vec3(0.8f,0.2f,0.3f);
        float lightAmbientIntensity = 0.2f;
        vec3 ambientShade = boxAmbientCoff * lightAmbientIntensity;


        vec3 boxDiffuseCoff = vec3(0.8f,0.2f,0.3f);
        float lightIntensity = 0.8f;
        vec3 l = normalize(lightPos - intersectionPoint);
        vec3 lambertionShade = boxDiffuseCoff * lightIntensity * max(0.0f, dot(normal, l));

        vec3 boxSpecularCoff = vec3(1.0f,1.0f,1.0f);
        vec3 v = normalize(camRay.origin - intersectionPoint);
        vec3 halfVector = (v + l) / length(v+l);
        float phongExponent = 10.0f;
        vec3 specularShader = boxSpecularCoff * lightIntensity * pow( max(0.0f, dot(normal, halfVector)), phongExponent );

        //Now we can do basic lighting on the cube (no ray traced lighting since we only have one object)
        FragColor = vec4(ambientShade + lambertionShade + specularShader,0);
    }
    else {
        FragColor = vec4(.71f, .91f, .93f, 1.0f);
    }
}

