* [33mcommit 4554ee755a3da776d2d2f7153404af0dd0a3907b[m[33m ([m[1;36mHEAD[m[33m -> [m[1;32mraytraced-reflections-implementation[m[33m, [m[1;31morigin/raytraced-reflections-implementation[m[33m)[m
[31m|[m Author: Jim <jimvdpiep@gmail.com>
[31m|[m Date:   Thu Oct 30 14:18:25 2025 +0100
[31m|[m 
[31m|[m     Changed attenuation to float to make it's use more clear
[31m|[m 
* [33mcommit 4fd7d7f662883a3c35e323e832af5ffb1e66def8[m
[31m|[m Author: Jim <jimvdpiep@gmail.com>
[31m|[m Date:   Tue Oct 28 14:03:27 2025 +0100
[31m|[m 
[31m|[m     Getting the indexes from the normal, metallic and roughness textures to the shader they are not yet used
[31m|[m 
* [33mcommit 42816a95136e7f0a951fd653e7f8e718e6672bb0[m
[31m|[m Author: Jim <jimvdpiep@gmail.com>
[31m|[m Date:   Mon Oct 27 20:10:01 2025 +0100
[31m|[m 
[31m|[m     orking hybrid reflections I used the normal to offset the origin which was and oversight by me we need to offset it in the direction of reflection vector
[31m|[m 
* [33mcommit f00f7e44ea6ffce2e64aab7b3ed02c29384b9f1d[m
[31m|[m Author: Jim <jimvdpiep@gmail.com>
[31m|[m Date:   Mon Oct 27 14:30:39 2025 +0100
[31m|[m 
[31m|[m     Added inverse view matrix to the UBO to convert view space normals to world space normals inside the raygen shader
[31m|[m 
* [33mcommit fe58d3eb0da55e85d4cd771e1a1368d21d7d0797[m
[31m|[m Author: Jim <jimvdpiep@gmail.com>
[31m|[m Date:   Mon Oct 27 12:18:14 2025 +0100
[31m|[m 
[31m|[m     Enabled specular render target when ray tracing is enabled, the specular rt has metallic data in it's W component which will be used to determine if a material is reflective
[31m|[m 
* [33mcommit e6949ac91d9547fc8ac54a20857fb7b9b50c7e78[m
[31m|[m Author: Jim <jimvdpiep@gmail.com>
[31m|[m Date:   Fri Oct 24 17:08:58 2025 +0200
[31m|[m 
[31m|[m     Reconstructed the world position from the depth render target and outputed as a color
[31m|[m 
* [33mcommit 952ed4036411148eb2c5e1639b0b92dcccbae9bb[m
[31m|[m Author: Jim <jimvdpiep@gmail.com>
[31m|[m Date:   Fri Oct 24 10:36:12 2025 +0200
[31m|[m 
[31m|[m     Added new render targets to the shader like depth and specular and displayed the unpacked normals as final color added point sampler so we can use a sampler
[31m|[m 
* [33mcommit b075a2670ceea23fbd76408014521aeddcb37d18[m
[31m|[m Author: Jim <jimvdpiep@gmail.com>
[31m|[m Date:   Wed Oct 22 21:49:01 2025 +0200
[31m|[m 
[31m|[m     Working normal render target inside raytrace shader by changing the depth_pas_mode we force the depth pre pass to render the depth, normal and rougness into the render targets
[31m|[m 
* [33mcommit ad2c503778581fbfad98da6f2d04749eaa3a30b2[m
[31m|[m Author: Jim <jimvdpiep@gmail.com>
[31m|[m Date:   Wed Oct 22 19:50:28 2025 +0200
[31m|[m 
[31m|[m     Different method of getting the normal texture which has better checks if it is availiable and more readability
[31m|[m 
* [33mcommit cd57c4bb45ba7b4767ca02ce1017a3de39468b75[m
[31m|[m Author: Jim <jimvdpiep@gmail.com>
[31m|[m Date:   Wed Oct 22 18:59:13 2025 +0200
[31m|[m 
[31m|[m     Made sure the last artifacts of the merge are removed
