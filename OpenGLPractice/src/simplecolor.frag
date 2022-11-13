#version 330 core //This shader runs for each pixel/fragment
out vec4 FragColor; //No inputs here, just a output color for each fragments

void main()
{
    FragColor = vec4(1.0f, 0.9f, 0.2f, 1.0f);
}


//Plan for a hybrid ray tracer/rasterizer
//Pass BB, other objects and camera in CAMERA SPACE! (for simplicity, camera is always 0,0).
//We can now draw a ray from (0,0) towards the FragCoord defined by the fragment shader and trace to see
//If we hit a voxel BB or something. If we hit a voxel, do voxel search for color. Otherwise, we can use UV
//Or other methods to calculate the color of that pixel. (so we're still technically ray tracing everything
//, but if we touch an
//Image for example, we switch to getting the information from the UV rather than ray trace the color from the mesh.)
//We are then ray casting everything to a bounding box level, but only ray tracing the voxel stuff. Other methods
//Can be used to calculate stuff like water, which we figure out is water when our ray hits a non voxel BB.

//To test, apparently the fragment shader interpolates to know which triangle it is in, so if we set each triangle to
//an integer value (0,1,2..) to classify what type of object it represents (voxel, water, image) etc, we can avoid
//ray tracing anything that isn't a voxel all together, and simply ONLY send rays towards the pixels that have a bounding
//box. We can set these triangle values as a vertex attribute, and pass through the vertex shader to the fragment shader
//This should make it much faster rendering, especially if a large part of the screen is rasterized, since less rays
//will be cast


//Design question, what to do about images? Should EVERYTHING be a voxel? are images breakable? Will this make players
//Use images (of potentially higher resolutions) as textures and remove the visual componentof the game?
//Either way, better to implement the separate types now anyway, since there will prob be *something* thats better
//as not a voxel. In teardown, they use decals for TV screens, but voxels for images. I think it would be cool to have
//Destructable higher resolution images, but maybe that will look weird anyway. Implement NON voxel decals
//and decide later what to do. Maybe water could also be voxel? SFX would probably not be though. So its difficult to
//make it all voxel style, so the attribute difference is important so not everything is raytraced. There really isnt
//any issue with the decal stuff aside from players using it to make detailed textures, but that's on them I guess?
//Also adding textures to stuff does not change the shape, so it will prob just look weird and most people would only
//use textures to add posters/logos to stuff. we could also restrict the minimum pixel size so its still "pixel style"
//but flat rather than be a full voxel. Nonetheless it would not be rendered the same way as the voxel bounding boxes
//(but maybe still raytraced? if lighting makes more sense for it? it still has a BB) so the separation makes sense
//to do now, but for now I'll still to rasterizing anything thats not a voxel BB.

//No matter if I decide to go full ray tracing/full voxels in the future, there WILL Be parts that get renderered
//differently than main voxel bounding boxes (like water or decals or anything flat), so even if those algorithms for
//rendering get replaced, they will be different rendering algorithms than the currently existing ones. I think I have
//OCD, i dont hav eto fully decide things in the 3 hours i ve been thinking about this today