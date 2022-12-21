#version 430 core //This shader runs for each pixel/fragment
out vec4 FragColor;

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;

struct cameraInfo {
    vec4 cameraPos;
    vec4 cameraDir;
    vec4 cameraUp;
    vec4 cameraRight;
};

struct clumpInfo {
    vec4 dimensions;
    mat4 transformationMatrix;
};

layout(std430, binding = 3) buffer inputCameraData
{
    cameraInfo inputCameraArr[];
};

layout(std430, binding = 4) buffer inputClumpData
{
    clumpInfo inputClumpArr[];
};



struct Ray {
    vec3 origin;
    vec3 direction;
};


bool searchClump(vec3 clumpIntersectionPoint, vec3 dir, inout vec3 foundIntersectionPoint){
    //Using similar strategy as in BB tracing we can multiply by reciprical rather than divide as to avoid -0 value.
    //Need to make sure any time we are using this t value though, we are using it for comparisons.
    vec3 clumpDims = inputClumpArr[0].dimensions.xyz;
    vec3 clumpCenter = inputClumpArr[0].transformationMatrix[3].xyz;

    int voxelExistence[8] = {0,1,1,0,0,1,1,0};

    //First we need to check which voxel our clumpintersectionPoint exists at.
    float minBoundX = clumpCenter.x - (clumpDims.x / 2.0f);
    int xVoxelCoord = max(0,int(floor(clumpIntersectionPoint.x - minBoundX)));
    if (xVoxelCoord > int(clumpDims.x) - 1) xVoxelCoord = int(clumpDims.x) - 1;

    float minBoundY = clumpCenter.y - (clumpDims.y / 2.0f);
    int yVoxelCoord = max(0,int(floor(clumpIntersectionPoint.y - minBoundY)));
    if (yVoxelCoord > int(clumpDims.y) - 1) yVoxelCoord = int(clumpDims.y) - 1;


    float minBoundZ = clumpCenter.z - (clumpDims.z / 2.0f);
    int zVoxelCoord = max(0,int(floor(clumpIntersectionPoint.z - minBoundZ)));
    if (zVoxelCoord > int(clumpDims.z) - 1) zVoxelCoord = int(clumpDims.z) - 1;

    int existenceIndex = zVoxelCoord * int(clumpDims.y) * int(clumpDims.x) + yVoxelCoord * int(clumpDims.x) + xVoxelCoord;


    foundIntersectionPoint = clumpIntersectionPoint;
    return voxelExistence[existenceIndex] == 1;

}

vec3 rotatePointAroundPoint(vec3 pointToRotate, vec3 rotationCenter, mat3 rotationMatrix, bool inverseRotation){
    vec3 translatedPoint = pointToRotate - rotationCenter;
    vec3 rotatedIntersectionPoint;

    if (inverseRotation){
        rotatedIntersectionPoint = inverse(rotationMatrix) * translatedPoint;
    }
    else {
        rotatedIntersectionPoint = rotationMatrix * translatedPoint;
    }
    return rotatedIntersectionPoint + rotationCenter;
}

vec3 rotateDirectionAroundPoint(vec3 directionToRotate, vec3 rotationCenter, mat3 rotationMatrix, bool inverseRotation){
    vec3 translatedDir = directionToRotate - rotationCenter;
    vec3 translatedOrigin = -rotationCenter;
    vec3 rotatedOrigin = inverse(rotationMatrix) * translatedOrigin;
    rotatedOrigin = rotatedOrigin + rotationCenter;

    vec3 rotatedDir;
    if (inverseRotation){
        rotatedDir = inverse(rotationMatrix) * translatedDir;
    }
    else {
        rotatedDir = rotationMatrix * translatedDir;
    }
    return rotatedDir + rotationCenter - rotatedOrigin;
}


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

    cameraInfo frameCameraInfo = inputCameraArr[0];

    camRay.origin = frameCameraInfo.cameraPos.xyz;

    vec2 resolution = vec2(1920,1080);
    vec2 aspectRatio = vec2(1.77777777778f,1.0f);
    vec2 uv = (gl_FragCoord.xy / resolution.xy )*2.0 - 1.0;
    float u = aspectRatio.x * ( (float(gl_FragCoord.x) + 0.5f) / float(resolution.x)) - float(aspectRatio.x)/2.0f;
    float v = aspectRatio.y * ( (float(gl_FragCoord.y) + 0.5f) / float(resolution.y)) - float(aspectRatio.y)/2.0f;
    uv.x *= aspectRatio.x;
    float distance = 1.0f; //TODO: Pass this value in
    camRay.direction = normalize(distance*frameCameraInfo.cameraDir.xyz + (u * frameCameraInfo.cameraRight.xyz) + (v * frameCameraInfo.cameraUp.xyz));

    //For the formula from the textbook txmin = (xmin - xe)/xd, we write xd as dot(raydir, right) and xe as dot(right, delta), delta being center of BB - ray origin
    //When right = aligned x axis, this is equal to saying get the x component of the origin point and direction vector, but we're now saying get x component on a rotated axis
    //Hard coding for now

    mat4 transformMat = inputClumpArr[0].transformationMatrix;
    vec3 right = transformMat[0].xyz;
    vec3 up = transformMat[1].xyz;
    vec3 forward = transformMat[2].xyz;
    vec3 translate = transformMat[3].xyz;

    vec3 dimensions = inputClumpArr[0].dimensions.xyz;
    float xMax = dimensions.x/2;
    float yMax = dimensions.y/2;
    float zMax = dimensions.z/2;

    float xMin = -xMax;
    float yMin = -yMax;
    float zMin = -zMax;


    bool intersects;
    float epsilon = 0.001f;
    vec3 delta = translate - camRay.origin;
    //These calculations need to be positive in the corrosponding axis (right is x)

    float deltaX = dot(right, delta);
    float unalignedDirX = dot(right, camRay.direction);

    float deltaY = dot(up, delta);
    float unalignedDirY = dot(up, camRay.direction);

    float deltaZ = dot(forward, delta);
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

    vec3 normal;
    bool enteredXPos;
    bool enteredYPos;
    bool enteredZPos;

    if (a_x >= 0) {
        enteredXPos = true;
        t_min_x = a_x * (xMin + deltaX);
        t_max_x = a_x * (xMax + deltaX);
    } else {
        enteredXPos = false;
        t_max_x = a_x * (xMin + deltaX);
        t_min_x = a_x * (xMax + deltaX);
    }

    if (a_y >= 0) {
        enteredYPos = true;
        t_min_y = a_y * (yMin + deltaY);
        t_max_y = a_y * (yMax + deltaY);
    } else {
        enteredYPos = false;
        t_max_y = a_y * (yMin + deltaY);
        t_min_y = a_y * (yMax + deltaY);
    }

    if (a_z >= 0) {
        enteredZPos = true;

        t_min_z = a_z * (zMin + deltaZ);
        t_max_z = a_z * (zMax + deltaZ);
    } else {
        enteredZPos = false;
        t_max_z = a_z * (zMin + deltaZ);
        t_min_z = a_z * (zMax + deltaZ);
    }

    bool enteredXFirst = false;
    bool enteredYFirst = false;
    bool enteredZFirst = false;
    if (t_min_y > t_min_z && t_min_y > t_min_x){
        enteredYFirst = true;
    }
    else if (t_min_x > t_min_z && t_min_x > t_min_y){
        enteredXFirst = true;
    }
    else {
        enteredZFirst = true;
    }

    float t_min = max(t_min_x, max(t_min_y, t_min_z));
    float t_max = min(t_max_x, min(t_max_y, t_max_z));

    if (t_min <= t_max) {
        if (t_min > 100000) {
            intersects = false;
            return;
        } else if (t_max < 0) {
            intersects = false;
        } else intersects = true;
    } else {
        intersects = false;
    }


    if (intersects){
        //If we get here, we know that the ray intersects the bounding box, and
        // we can check the intersection point as being t_min
        //Now we must check if it hits any voxels in the BB.

        mat3 rotationMat = mat3(transformMat[0].xyz, transformMat[1].xyz, transformMat[2].xyz);
        vec3 intersectionPoint = camRay.origin + (t_min * camRay.direction);

        //Note its much easier to simply rotate the intersection point, but rotating the viewing ray allow us to
        //use this ray to query which voxels in the clump we can see (so we'd have to rotate the ray anyway)

        //Method: Rotate the ray origin and direction around the center point of a clump, then the clump will be axis
        //aligned and we can see which side of the clump it entered by comparing intersection point to planes.
        // We can skip this by rotating the intersection point or just use the above information, but since we
        // will need to rotate the ray anyway to query the inner voxels, we can use that information here as well.
        // After voxel search is implemented, see if any part of this is unused and replace with simpler methods (either
        // using above info for normals or rotating intersection point if rotated ray is not needed).

        //Another note: currently using cam direction and intersection point rotation, no camera origin. Since camera
        //origin is no longer needed and can be confusing since its not really our camera. We have intersection point going
        //in and well as can easily calculate going out. then we can set up "new" origins with our given direction.
        //so the original camera origin is no longer part of it, since our intersection and camera direction tell us
        //everything we need to know (we're still tehcnically using camera origin to determine this dir)

        vec3 worldAlignedIntersectionPoint = rotatePointAroundPoint(intersectionPoint, translate, rotationMat, true);
        vec3 worldAlignedRayDirection = rotateDirectionAroundPoint(camRay.direction, translate, rotationMat, true);

        vec3 normal;


        if (abs(worldAlignedIntersectionPoint.x - (translate.x + xMin)) < epsilon){
            normal = -1 * right;
        }
        else if (abs(worldAlignedIntersectionPoint.x - (translate.x + xMax)) < epsilon){
            normal = right;
        }
        else if (abs(worldAlignedIntersectionPoint.y - (translate.y + yMin)) < epsilon){
            normal = -1 * up;
        }
        else if (abs(worldAlignedIntersectionPoint.y - (translate.y + yMax)) < epsilon){
            normal = up;
        }
        else if (abs(worldAlignedIntersectionPoint.z - (translate.z + zMin)) < epsilon){
            normal = -1 * forward;
        }
        else if (abs(worldAlignedIntersectionPoint.z - (translate.z + zMax)) < epsilon){
            normal = forward;
        }
        else {
            FragColor = vec4(.01f, .11f, .93f, 1.0f);
            return;
        }

        vec3 worldAlginedVoxelIntersectionPoint = vec3(0,0,0);

        if(!searchClump(worldAlignedIntersectionPoint, worldAlignedRayDirection, worldAlginedVoxelIntersectionPoint)){
            FragColor = vec4(.71f, .91f, .93f, 1.0f);
            return;
        };

        vec3 voxelIntersectionPoint = rotatePointAroundPoint(worldAlginedVoxelIntersectionPoint, translate, rotationMat, false);
        //Below is a point light, if we want a directional light, the l is simply a unit vector pointing towards
        // that directional light
        vec3 lightPos = vec3(1.0f,3.0f,3.0f);
        vec3 boxAmbientCoff = vec3(0.8f,0.2f,0.3f);
        float lightAmbientIntensity = 0.2f;
        vec3 ambientShade = boxAmbientCoff * lightAmbientIntensity;


        vec3 boxDiffuseCoff = vec3(0.8f,0.2f,0.3f);
        float lightIntensity = 0.8f;
        vec3 l = normalize(lightPos - voxelIntersectionPoint);
        vec3 lambertionShade = boxDiffuseCoff * lightIntensity * max(0.0f, dot(normal, l));

        vec3 boxSpecularCoff = vec3(1.0f,1.0f,1.0f);
        vec3 v = normalize(camRay.origin - voxelIntersectionPoint);
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

