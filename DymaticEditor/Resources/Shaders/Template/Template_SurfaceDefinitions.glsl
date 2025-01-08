CompilerInclude(Template_ConstantDefinitions)
CompilerDefine(World Position, SURFACE_POSITION);
CompilerDefine(World Normal, SURFACE_NORMAL);
CompilerDefine(Vertex Color, SURFACE_COLOR);
CompilerDefine(Vertex Depth, gl_FragCoord.z);
CompilerDefine(Vertex Linear Depth, LinearDepth(gl_FragCoord.z));
CompilerDefine(Texture Coordinates, SURFACE_TEXTURE_COORD);
CompilerDefine(Object Position, (u_Model[3].xyz));
CompilerDefine(Pixel Position, vec2(gl_FragCoord.xy) / vec2(u_ScreenDimensions));
CompilerDefine(Entity ID, float(u_EntityID));
CompilerDefine(Submesh Index, float(u_SubmeshIndex));
CompilerDefine(Vertex Index, float(Input.GlobalIndex));