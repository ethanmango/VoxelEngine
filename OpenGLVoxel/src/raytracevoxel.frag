#version 430 core

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
    //Need to find a better way to include this information as a variable length
    // should be generated in the GPU so that one color array is passed in for all
    //models in a scene..

    //For voxels, put all of them in one long voxel array that gets passed into the SSBO separately
    //so that a variable size can be used. use IDS and voxelSizes to skip to the needed part in the array
    //Then the color/other data can come in a separate SSBO as well so it can  be variable size, as the needed
    //number of colors depends on the vox files imported so the CPU will combine into an unknown sized color array
    //and pass it in. This also means we can remove any colors that aren't being used by any voxel.. the question
    //is how to make this efficient as to not keep changing this array frame by frame. Maybe a world load thing that
    //knows at the beginning which colors are needed
    vec4 colors[256];

};

layout(std430, binding = 3) buffer inputCameraDataBuff
{
    cameraInfo inputCameraArr[];
};

layout(std430, binding = 4) buffer inputClumpDataBuff
{
    clumpInfo inputClumpArr[];
};
layout(std430, binding = 5) buffer inputVoxelInfoBuff
{
    int inputVoxelInfo[];
};



struct Ray {
    vec3 origin;
    vec3 direction;
};

struct clumpNormals {
    vec3 right;
    vec3 up;
    vec3 forward;
};

bool voxelExists(vec3 dimensions, int x, int y, int z){
    if (x < 0 || y < 0 || z < 0){
        return false;
    }
    if (x >= dimensions.x || y >= dimensions.y || z >= dimensions.z){
        return false;
    }
    return true;

}
int existenceCoords(vec3 dims, int x, int y, int z) {
    return z * int(dims.y) * int(dims.x) + y * int(dims.x) + x;
}

//NOTE: When rewriting this code in rust, look into passing in structs rather than everything like this
bool searchClump(vec3 clumpIntersectionPoint, vec3 dir, vec3 clumpIntersectionNormal, clumpNormals norms, inout vec3 foundIntersectionPoint, inout vec3 foundIntersectionNormal, bool firstCounted, vec3 inputDir, inout vec3 outputColor){
    //Using similar strategy as in BB tracing we can multiply by reciprical rather than divide as to avoid -0 value.
    //Need to make sure any time we are using this t value though, we are using it for comparisons.
    vec3 clumpDims = inputClumpArr[0].dimensions.xyz;
    vec3 clumpCenter = inputClumpArr[0].transformationMatrix[3].xyz;



    //First we need to check which voxel our clumpintersectionPoint exists at.

    //For points starting within a clump, we need to define how we select which voxel we're in (this is based on the out normal)
    //but we can probably make a uniform algorithm based on input normal.. below works for outer voxels

    //For rust implementation, split up this implementation so that for shadow rays all we need to check is the existence of a another voxel in the way and not need to recalculate stuff like intersection point and voxels
    //For this the innermost function can calculate true or false for a intersection, then pass into outer functions the results for recalculation of the intersection points and normals.

    float epsilon = 0.000001f;
    //Pass in a inputDir vec3 of (0,0,1) where ONE value isn't 0, and that value is either 1 or -1 representing the direction the intersection comes in. (useful for non edge voxel intersection points);
    //Move the intersection point .000001 in that direction.. (may cause problems around edges... we'll see???
    float minBoundX = clumpCenter.x - (clumpDims.x / 2.0f);
    clumpIntersectionPoint.x += -1 * epsilon * inputDir.x;
    int xVoxelCoord = max(0,int(floor(clumpIntersectionPoint.x - minBoundX)));
    if (xVoxelCoord > int(clumpDims.x) - 1) xVoxelCoord = int(clumpDims.x) - 1;

    float minBoundY = clumpCenter.y - (clumpDims.y / 2.0f);
    clumpIntersectionPoint.y += -1 * epsilon * inputDir.y;

    int yVoxelCoord = max(0,int(floor(clumpIntersectionPoint.y - minBoundY)));
    if (yVoxelCoord > int(clumpDims.y) - 1) yVoxelCoord = int(clumpDims.y) - 1;


    float minBoundZ = clumpCenter.z - (clumpDims.z / 2.0f);
    clumpIntersectionPoint.z += -1 * epsilon * inputDir.z;

    int zVoxelCoord = max(0,int(floor(clumpIntersectionPoint.z - minBoundZ)));
    if (zVoxelCoord > int(clumpDims.z) - 1) zVoxelCoord = int(clumpDims.z) - 1;

    int existenceIndex = existenceCoords(clumpDims, xVoxelCoord, yVoxelCoord, zVoxelCoord);


    //Return early if the starter voxel exists
    if (inputVoxelInfo[existenceIndex] != -1 && firstCounted) {
        foundIntersectionPoint = clumpIntersectionPoint;
        foundIntersectionNormal = clumpIntersectionNormal;
        int colorIndex = inputVoxelInfo[existenceIndex];
        outputColor = inputClumpArr[0].colors[colorIndex].xyz;
        return true;
    };

    //Now that we have the voxel that we start in, we can start the traversal algorithm
    ///First we get the step direction
    vec3 stepDir;
    if (dir.x >= 0) stepDir.x = 1;
    else stepDir.x = -1;
    if (dir.y >= 0) stepDir.y = 1;
    else stepDir.y = -1;
    if (dir.z >= 0) stepDir.z = 1;
    else stepDir.z = -1;

    //Next we get the tDelta, or how far along the ray we need to go to cross an entire voxel

    vec3 tDelta;
    //Need to take absolute value as delta is a scalar and direction doesn't matter
    tDelta.x = abs(1.0f/dir.x);
    tDelta.y = abs(1.0f/dir.y);
    tDelta.z = abs(1.0f/dir.z);

    //Now we initalize tMax, which is how far along the ray to get to a certain voxel in each direction.
    //These all start out as how far until the first next voxel, then get tDelta added to it each time after

    vec3 tMax;
    vec3 comparisonLine;

    comparisonLine.x = minBoundX + xVoxelCoord;
    comparisonLine.y = minBoundY + yVoxelCoord;
    comparisonLine.z = minBoundZ + zVoxelCoord;

    if (stepDir.x == 1) comparisonLine.x = comparisonLine.x + 1;
    if (stepDir.y == 1) comparisonLine.y = comparisonLine.y + 1;
    if (stepDir.z == 1) comparisonLine.z = comparisonLine.z + 1;

    //Abs value here again as t is a scalar and direction doesn't matter
    tMax.x = tDelta.x * abs(comparisonLine.x - clumpIntersectionPoint.x);
    tMax.y = tDelta.y * abs(comparisonLine.y - clumpIntersectionPoint.y);
    tMax.z = tDelta.z * abs(comparisonLine.z - clumpIntersectionPoint.z);

    float closestTMax = 0;
    int closestTMaxAxis = 1;
    //Now we are done with the initialization step, and can move onto the iterative.
    while (voxelExists(clumpDims, xVoxelCoord, yVoxelCoord, zVoxelCoord)){

        //Need to see if it exists first before we increment in case we go out of bounds
        existenceIndex = existenceCoords(clumpDims, xVoxelCoord, yVoxelCoord, zVoxelCoord);
        if (inputVoxelInfo[existenceIndex] != -1 && closestTMax != 0) {
            foundIntersectionPoint = clumpIntersectionPoint + closestTMax * dir;
            if (closestTMaxAxis == 0 && stepDir.x == 1) foundIntersectionNormal = -norms.right;
            else if (closestTMaxAxis == 0 && stepDir.x == -1) foundIntersectionNormal = norms.right;
            else if (closestTMaxAxis == 1 && stepDir.y == 1) foundIntersectionNormal = -norms.up;
            else if (closestTMaxAxis == 1 && stepDir.y == -1) foundIntersectionNormal = norms.up;
            else if (closestTMaxAxis == 2 && stepDir.z == 1) foundIntersectionNormal = -norms.forward;
            else if (closestTMaxAxis == 2 && stepDir.z == -1) foundIntersectionNormal = norms.forward;
            int colorIndex = inputVoxelInfo[existenceIndex];
            outputColor = inputClumpArr[0].colors[colorIndex].xyz;
            return true;
        };

        if (tMax.x <= tMax.y && tMax.x <= tMax.z){
            xVoxelCoord += int(stepDir.x);
            closestTMax = tMax.x;
            closestTMaxAxis = 0;
            tMax.x += tDelta.x;


        }
        else if (tMax.y <= tMax.x && tMax.y <= tMax.z){
            yVoxelCoord += int(stepDir.y);
            closestTMax = tMax.y;
            closestTMaxAxis = 1;
            tMax.y += tDelta.y;


        }
        else {
            zVoxelCoord += int(stepDir.z);
            closestTMax = tMax.z;
            closestTMaxAxis = 2;
            tMax.z += tDelta.z;


        }
    }

    return false;


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

    clumpNormals clumpNorms;
    clumpNorms.right = right;
    clumpNorms.up = up;
    clumpNorms.forward = forward;

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
            return;
        }


        vec3 worldAlginedVoxelIntersectionPoint = vec3(0,0,0);
        vec3 intersectionNormal = vec3(1,1,0);
        vec3 worldAlignedClumpNormal = rotateDirectionAroundPoint(normal, translate, rotationMat, true);
        vec3 voxelColor = vec3(0,0,0);
        if(!searchClump(worldAlignedIntersectionPoint, worldAlignedRayDirection, normal, clumpNorms, worldAlginedVoxelIntersectionPoint, intersectionNormal, true, worldAlignedClumpNormal, voxelColor)){
            FragColor = vec4(.21f, .11f, .13f, 0.0f);
            return;
        };
        vec3 voxelIntersectionPoint = rotatePointAroundPoint(worldAlginedVoxelIntersectionPoint, translate, rotationMat, false);
        vec3 worldAlignedIntersectionNormal =rotateDirectionAroundPoint(intersectionNormal, translate, rotationMat, true);



        //Check if voxel is shadowed by another voxel in the same clump. First we rotate the light source to match the clump

        //Below is a point light, if we want a directional light, the l is simply a unit vector pointing towards
        // that directional light
        vec3 lightPositions[2];
        lightPositions[0] = vec3(-50.0f,50.0f, -50.0f);
        lightPositions[1] = vec3(50.0f,50.0f, 50.0f);

        vec3 boxAmbientCoff = voxelColor;
        float lightAmbientIntensity = 0.2f;
        vec3 ambientShade = boxAmbientCoff * lightAmbientIntensity;



        FragColor = vec4(ambientShade,1);

        for(int i = 0; i < 2; i++){
            vec3 clumplightPos = rotatePointAroundPoint(lightPositions[i], translate, rotationMat, false);
            vec3 dirToLight = normalize(lightPositions[i] - voxelIntersectionPoint);
            vec3 worldAlignedDirToLight = rotateDirectionAroundPoint(dirToLight, translate, rotationMat, true);
            bool shadow = false;

            vec3 test1 = vec3(0,0,0);
            vec3 test2 = vec3(0,0,0);
            vec3 unusedVoxColor;


            if(searchClump(worldAlginedVoxelIntersectionPoint, worldAlignedDirToLight, vec3(0,0,0), clumpNorms, test1, test2, false, worldAlignedIntersectionNormal, unusedVoxColor)) {
                shadow = true;
            }


            //For now we will only define the ambient/diffuse when we pass in a color, the rest we figure out later
            //        vec3 boxDiffuseCoff = vec3(0.8f,0.2f,0.3f);
            float lightIntensity = 0.8f;
            vec3 l = normalize(lightPositions[i] - voxelIntersectionPoint);
            vec3 lambertionShade = boxAmbientCoff * lightIntensity * max(0.0f, dot(intersectionNormal, l));

            //Below defines the color of the shinyness.. don't know if this should be a color or an intensity and if it belongs tot he voxel or the light.. look into this later
            //specular stuff also looks kinda weird so look into that..
            vec3 boxSpecularCoff = vec3(1.0f,1.0f,1.0f);
            vec3 v = normalize(camRay.origin - voxelIntersectionPoint);
            vec3 halfVector = (v + l) / length(v+l);
            float phongExponent = 10.0f;
            vec3 specularShader = boxSpecularCoff * lightIntensity * pow( max(0.0f, dot(intersectionNormal, halfVector)), phongExponent );


            //Now we can do basic lighting on the cube (no ray traced lighting since we only have one object)

            if(!shadow) {
                FragColor += vec4(lambertionShade ,0);
            }
        }

    }
    else {
        FragColor = vec4(.11f, .91f, .93f, 0.0f);
    }
}

