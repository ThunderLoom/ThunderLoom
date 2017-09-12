#pragma once
#include "thunderloom.h"
tlWeaveParameters *tl_pattern_editor(tlWeaveParameters *param);

// -- Usage --
//In one of your source files, define TL_PATTERN_EDITOR_IMPLEMENTATION
//before including this file, like so:
//
//#define TL_PATTERN_EDITOR_IMPLEMENTATION
//#include "pattern_editor.h"
//
// -- Dependencies --
// The pattern editor depends on dear imgui, glfw and gl3w.
// See README.txt in ../dependencies for instructions on how to get these
// libraries

#ifdef TL_PATTERN_EDITOR_IMPLEMENTATION

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <commdlg.h>
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_MAC
//For macOS a small wrapper for Cocoa features (alert, save, open) is required
//implementation in frontends/standalone_pattern_editor/macos_wrapper.mm
extern void macos_open_file_dialog(char * buffer, int buffersize,
        const char * adefaultpathandfile, const char (*filters)[10],
        const int n_filters);
extern void macos_save_file_dialog(char * buffer, int buffersize,
        const char * defaultpathandfile, const char * extention);
extern int macos_alert(const char * details, const char * msg);
    #endif
#endif
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include "imgui.h"
#include "GL/gl3w.h"
#include "GLFW/glfw3.h"
#include "imgui_impl_glfw_gl3.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"

static const char * pattern_vert = 
    "#version 330\n"
    "uniform mat4 ProjMtx;\n"
    "in vec2 Position;\n"
    "in vec2 UV;\n"
    "out vec2 Frag_UV;\n"
    "void main()\n"
    "{\n"
    "    Frag_UV = UV;\n"
    "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
    "}";

static const char * pattern_frag = 
"#version 330\n"
"#define M_PI 3.1415926535897932384626433832795\n"
"uniform sampler2D Texture;\n"
"uniform float w;\n"
"uniform float h;\n"
"uniform float alpha;\n"
"in vec2 Frag_UV;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
    "float thickness = 0.01f;\n"
    "float aa = 0.02f;\n"
    "vec2 uv;\n"
    "vec2 frac;\n"
    "frac.x = modf(Frag_UV.s*w,uv.x);\n"
    "frac.y = modf(Frag_UV.t*h,uv.y);\n"
    "uv = (uv + vec2(0.5f,0.5f))/vec2(w,h);\n"
    "color = texture(Texture, uv);\n"
    "float v = 0.4;\n"
    "float dir = color.a;\n"
    "float x = frac.x*dir + frac.y*(1.f-dir);\n"
    "float y = frac.y*dir + frac.x*(1.f-dir);\n"
    "color *= (1.f-v) + v*sin(M_PI*x);\n"
    "float xx = 0.5f-abs(x-0.5f);\n"
    "color *= smoothstep(thickness,thickness+aa,xx);\n"
    "vec2 offset = dir < 0.5f ? vec2(1.f/w,0.f) : vec2(0.f,1.f/h);\n"
    "vec4 s = texture(Texture,uv - offset);\n"
    "if(abs(s.a - dir) > 0.2f){\n"
        "color *= smoothstep(thickness,thickness+aa,y);\n"
    "}\n"
    "vec4 s2 = texture(Texture,uv + offset);\n"
    "if(abs(s2.a - dir) > 0.2f){\n"
        "color *= 1.f - smoothstep(1.f - thickness - aa,\n"
            "1.f - thickness,y);\n"
    "}\n"
    "color *= alpha;\n"
"}\n";

static double scroll = 0.0; 

const int entry_size=20;

typedef struct{
    unsigned char *bitmap;
    int current_yarn_type;
    int mouse_state;
    int warp;
    int tile;
    int controls_offset;
    int mouse_drag_x, mouse_drag_y;
    int w,h;
    float center_x, center_y; // The position in the weaving draft that is drawn
                              // in the center of the canvas. In the range 0 - 1
    float magnification; // The number of pixels that is shown in the canvas
                         // (across the smallest axis)
} EditorData;

static void pattern_editor_set_imgui_style();

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

static const char *open_file_dialog_filters=
    "Pattern Files | *.WIF;*.PTN\0*.WIF;*.PTN\0"
    "All Files | *.*\0*.*\0"
    ;

static const char *save_file_dialog_filters=
    "PTN Files | *.PTN\0*.PTN\0"
    "All Files | *.*\0*.*\0"
    ;

static char* file_open_dialog(const char *filters,const char* title, bool save)
{
    char *buffer = (char*)calloc(512,sizeof(char));
#ifdef WIN32
    OPENFILENAMEA openfilename={0};
    openfilename.lStructSize=sizeof(OPENFILENAMEA);
    openfilename.hwndOwner=NULL;
    openfilename.lpstrFilter=filters;
    openfilename.lpstrFile=buffer;
    openfilename.lpstrDefExt="ptn";
    openfilename.nMaxFile=512;
    openfilename.lpstrTitle=title;
    if(save){
        GetSaveFileNameA(&openfilename);
    }else{
        GetOpenFileNameA(&openfilename);
    }
#elif TARGET_OS_MAC
    const char t_filters[2][10] = {"wif", "ptn"};
    const char *suggested_path = ""; 
    const char *extention = ".ptn"; 
    if(save){
        macos_save_file_dialog(buffer, 512, suggested_path, extention);
    }else{
        macos_open_file_dialog(buffer, 512, suggested_path, t_filters, 2);
    }
#elif __linux__
    //TODO: filedialogs for linux (gtk?)
    printf("ERROR: linux file dialogs not implemented! :( \n");
#endif
    
    return buffer;
}


static void save_ptn_file(const char *filename,tlWeaveParameters *param){
    long len=0;
    unsigned char *data = tl_pattern_to_ptn_file(param,&len);
    FILE *fp = fopen(filename,"wb");
    fwrite(data,len,1,fp);
    fclose(fp);
    free(data);
}


static void check_compilation(GLuint handle, GLenum flag)
{
    GLint ret;
    if(flag==GL_COMPILE_STATUS){
        glGetShaderiv(handle,flag,&ret);
    }
    if(flag==GL_LINK_STATUS){
        glGetProgramiv(handle,flag,&ret);
    }
    if(ret!=GL_TRUE){
        char buffer[1024];
        GLsizei len;
        if(flag==GL_COMPILE_STATUS){
            glGetShaderInfoLog(handle,1024,&len,buffer);
        }
        if(flag==GL_LINK_STATUS){
            glGetProgramInfoLog(handle,1024,&len,buffer);
        }
        buffer[len]=0;
        printf("Error: %s\n",buffer);
#ifdef WIN32
        OutputDebugStringA("ERROR:\n");
        OutputDebugStringA(buffer);
#endif
    }
}

static void get_projection_matrix(float *mat, int display_w, int display_h,
    int panel_width, float image_x, float image_y, float zoom)
{
        float scale=2.f*powf(2.f,zoom);
        float offset_x=(float)panel_width/(float)display_w;
        float display_aspect=(float)display_h/(float)display_w;
        float canvas_aspect=(float)display_h/(float)(display_w-panel_width);
        float img_aspect=1.f;
        float aspect=display_aspect/img_aspect;
        float aspect2=canvas_aspect/img_aspect;
        if(aspect2>1.f){
            scale/=aspect2;
        }
        float proj[4][4]=
        {
            { scale*aspect,  0.0f, 0.0f, 0.0f },
            { 0.0f, -scale, 0.0f, 0.0f },
            { 0.0f,  0.0f, 1.0f, 0.0f },
            {(-0.5f + image_x)*scale*aspect + offset_x, (0.5f -image_y)*scale, 0.0f, 1.0f },
        };
        memcpy(mat,proj,16*sizeof(float));
}


static int compile_shader(const char *vert_shader, const char *frag_shader)
{
    int shader_handle = glCreateProgram();
    int vert_handle = glCreateShader(GL_VERTEX_SHADER);
    int frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vert_handle, 1, &vert_shader, 0);
    glShaderSource(frag_handle, 1, &frag_shader, 0);
    glCompileShader(vert_handle);
    check_compilation(vert_handle,GL_COMPILE_STATUS);
    glCompileShader(frag_handle);
    check_compilation(frag_handle,GL_COMPILE_STATUS);
    glAttachShader(shader_handle, vert_handle);
    glAttachShader(shader_handle, frag_handle);
    glLinkProgram(shader_handle);
    check_compilation(shader_handle, GL_LINK_STATUS);
    return shader_handle;
}

static void draw_pattern_texture(unsigned char *bitmap, tlWeaveParameters *param)
{
    int w=param->pattern_width;
    int h=param->pattern_height;
    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            PatternEntry pe=param->pattern[x+y*w];
            tlYarnType yt=param->yarn_types[pe.yarn_type];
            if(!yt.color_enabled){
                yt=param->yarn_types[0];
            }
            unsigned char r = yt.color.r*255.f;
            unsigned char g = yt.color.g*255.f;
            unsigned char b = yt.color.b*255.f;
            unsigned char a = pe.warp_above*255.f;
            unsigned char *p = bitmap + (x+y*w)*4;
            p[0]=r; p[1]=g; p[2]=b; p[3] = a;
        }
    }
}

static void redraw_pattern(EditorData *data, tlWeaveParameters *param, GLuint pattern_tex)
{
    draw_pattern_texture(data->bitmap, param);
    glBindTexture(GL_TEXTURE_2D,pattern_tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,param->pattern_width,param->pattern_height,0,
        GL_RGBA,GL_UNSIGNED_BYTE,data->bitmap);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    scroll += yoffset;
    ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
}

tlWeaveParameters *tl_pattern_editor(tlWeaveParameters *param)
{
        // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return param;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Pattern editor", NULL, NULL);
    glfwMakeContextCurrent(window);
    gl3wInit();

    EditorData data={};
    data.bitmap=(unsigned char*)calloc(param->pattern_width*param->pattern_height,4);
    data.center_x = 0.5f;
    data.center_y = 0.5f;
    data.current_yarn_type=1;
    data.warp = 1;
    data.tile = 0;
    draw_pattern_texture(data.bitmap,param);

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true);
    glfwSetScrollCallback(window,scroll_callback);

    ImVec4 clear_color = ImColor(35, 35, 35);
    pattern_editor_set_imgui_style();

    int pattern_shader = compile_shader(pattern_vert,pattern_frag);

    GLuint vert_array;
    glGenVertexArrays(1,&vert_array);
    glBindVertexArray(vert_array);
    static const GLfloat vertex_buffer_data[]={
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    };

    GLuint vertexbuffers[3];
    glGenBuffers(3,vertexbuffers);
    glBindBuffer(GL_ARRAY_BUFFER,vertexbuffers[0]);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertex_buffer_data),vertex_buffer_data,GL_STATIC_DRAW);
    int attrib_pos = glGetAttribLocation(pattern_shader,"Position");
    int attrib_uv = glGetAttribLocation(pattern_shader,"UV");
    int attrib_tex = glGetUniformLocation(pattern_shader,"Texture");
    int attrib_matrix = glGetUniformLocation(pattern_shader,"ProjMtx");
    int attrib_w = glGetUniformLocation(pattern_shader,"w");
    int attrib_h = glGetUniformLocation(pattern_shader,"h");
    int attrib_alpha = glGetUniformLocation(pattern_shader,"alpha");

    GLuint pattern_tex;
    glGenTextures(1,&pattern_tex);
    glBindTexture(GL_TEXTURE_2D,pattern_tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,param->pattern_width,param->pattern_height,0,
        GL_RGBA,GL_UNSIGNED_BYTE,data.bitmap);

    bool open = true;
    float last_cursor_x = 0.f;
    float last_cursor_y = 0.f;
    float image_x = 0.f;
    float image_y = 0.f;
    float zoom = 0.f;
    bool dragging = false;
    bool zooming = false;
    float projection[16];
    int panel_width=240;
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    get_projection_matrix(projection,display_w,display_h,
        panel_width,image_x,image_y,zoom);

    ImGuiIO& io = ImGui::GetIO();
    int size[2] = {(int)param->pattern_width,(int)param->pattern_height};

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        glfwGetFramebufferSize(window, &display_w, &display_h);

        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(panel_width,display_h));
            ImGui::Begin("Controls",&open,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);
            if(ImGui::Button("Load pattern")){
                char *filename = file_open_dialog(open_file_dialog_filters,"Load pattern",false);
                if(filename[0] != 0){
                    const char *error = 0;
                    tlWeaveParameters* p =
                    tl_weave_pattern_from_file(filename,&error);
                    if(p){
                        tl_free_weave_parameters(param);
                        param = p;
                        data.center_x = 0.5f;
                        data.center_y = 0.5f;
                        data.current_yarn_type=1;
                        free(data.bitmap);
                        data.bitmap=(unsigned char*)calloc(param->pattern_width*param->pattern_height,4);
                        redraw_pattern(&data,param,pattern_tex);
                        size[0] = param->pattern_width;
                        size[1] = param->pattern_height;
                    }else{
#ifdef WIN32
                        MessageBoxA(NULL,error,"ERROR!",MB_OK|MB_ICONERROR);
#elif TARGET_OS_MAC
                        macos_alert(error, "ERROR!");
#endif
                    }
                }
                free(filename);
            }
            if(ImGui::Button("Save pattern")){
                char *filename = file_open_dialog(save_file_dialog_filters,"Save pattern",true);
                if(filename[0] != 0){
                    save_ptn_file(filename,param);
                }
                free(filename);
            }
            ImGui::Checkbox("Draw tiled",(bool*)&data.tile);
            if(ImGui::DragInt2("Size",size,0.1f)){
                size[0] = size[0] <= 0 ? 1 : size[0];
                size[1] = size[1] <= 0 ? 1 : size[1];
                PatternEntry *pe = (PatternEntry*)calloc(size[0]*size[1],sizeof(PatternEntry));
                int min_w = size[0] < param->pattern_width  ? size[0] : param->pattern_width;
                int min_h = size[1] < param->pattern_height ? size[1] : param->pattern_height;
                for(int y=0;y<size[1];y++){
                    for(int x=0;x<size[0];x++){
                        if(x<min_w && y<min_h){
                            pe[x+y*size[0]] = param->pattern[x+y*param->pattern_width];
                        }else{
                            pe[x+y*size[0]].yarn_type = 1;
                        }
                    }
                }
                free(param->pattern);
                param->pattern = pe;
                param->pattern_width  = size[0];
                param->pattern_height = size[1];
                free(data.bitmap);
                data.bitmap=(unsigned char*)calloc(param->pattern_width*param->pattern_height,4);
                redraw_pattern(&data,param,pattern_tex);
            }
            ImGui::Columns(2, NULL, false);
            static bool selected[16] = { 0 };
            if (ImGui::Selectable("Warp",data.warp==1)) {data.warp = data.warp==1 ? -1 : 1;}
            ImGui::NextColumn();
            if (ImGui::Selectable("Weft",data.warp==0)) {data.warp = data.warp==0 ? -1 : 0;}
            ImGui::Columns(1);
            {
                char buffer[256];
                for(int i=1;i<param->num_yarn_types;i++){
                    sprintf(buffer,"Yarn %d",i);
                    ImGui::PushID(buffer);
                    if(ImGui::Selectable(buffer,data.current_yarn_type==i)) {
                        data.current_yarn_type = data.current_yarn_type == i? 0 : i;
                    }
                    tlYarnType *yt = param->yarn_types;
                    if(ImGui::Checkbox("",(bool*)&param->yarn_types[i].color_enabled)){
                        redraw_pattern(&data,param,pattern_tex);
                    }
                    ImGui::SameLine();
                    if(ImGui::ColorEdit3("",(float*)&(yt+i)->color)){
                        redraw_pattern(&data,param,pattern_tex);
                    }
                    if(param->num_yarn_types>2){
                        ImGui::SameLine();
                        if(ImGui::Button("x")){
                            int w=param->pattern_width;
                            int h=param->pattern_height;
                            for(int y=0;y<h;y++){
                                for(int x=0;x<w;x++){
                                    PatternEntry *pe=param->pattern + x+y*w;
                                    if(pe->yarn_type >= i && pe->yarn_type > 1){
                                        pe->yarn_type--;
                                    }
                                }
                            }
                            for(int j=i+1;j<param->num_yarn_types;j++){
                                yt[j-1] = yt[j];
                            }
                            param->num_yarn_types--;
                            if(data.current_yarn_type>=i){
                                data.current_yarn_type--;
                            }
                            redraw_pattern(&data,param,pattern_tex);
                        }
                    }
                    ImGui::PopID();
                }
                if(ImGui::Button("Add yarn type")){
                    int n =param->num_yarn_types;
                    tlYarnType *yt = (tlYarnType*)calloc(n+1,sizeof(tlYarnType));
                    memcpy(yt,param->yarn_types,n*sizeof(tlYarnType));
                    free(param->yarn_types);
                    param->yarn_types = yt;
                    data.current_yarn_type = param->num_yarn_types;
                    param->num_yarn_types++;
                    param->yarn_types[n] = tl_default_yarn_type;
                    param->yarn_types[n].color_enabled = 1;
                }
            }
            ImGui::End();
        }

        if(!io.WantCaptureMouse){
            double xpos,ypos;
            glfwGetCursorPos(window,&xpos,&ypos);
            xpos=(xpos)/display_w;
            ypos/=display_h;
            double delta_x=(xpos-last_cursor_x);
            double delta_y=(ypos-last_cursor_y);
            float scale=2.f*powf(2.f,zoom);
            zoom += scroll*0.1;
            scroll = 0.0;
            if(glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_LEFT)){
                int w = param->pattern_width;
                int h = param->pattern_height;
                double dx = 2.f*xpos-1.f;
                dx -= projection[12];
                dx /= projection[0];
                double dy = -(2.f*ypos -1.f);
                dy -= projection[13];
                dy /= projection[5];
                int x = (int)(dx*w);
                int y = (int)(dy*h);
                if(x>=0 && y>=0 && x<w && y < h){
                    PatternEntry *pe=param->pattern + x+y*w;
                    if(data.warp >= 0){
                        pe->warp_above = data.warp;
                    }
                    if(data.current_yarn_type>0){
                        pe->yarn_type = data.current_yarn_type;
                    }
                    redraw_pattern(&data,param,pattern_tex);
                }
            }
            if(glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_MIDDLE)){
                delta_x /= projection[0];
                delta_x *= 2;
                delta_y /= projection[5];
                delta_y *= -2;

                if(!dragging){
                    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
                }
                image_x+=delta_x;
                image_y+=delta_y;
                dragging=true;
            } else{
                if(dragging && !zooming){
                    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
                }
                dragging=false;
            }
            if(glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT)){
                if(!zooming){
                    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
                }
                zoom+=delta_x;
                zooming=true;
            } else{
                if(zooming && !dragging){
                    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
                }
                zooming=false;
            }
            last_cursor_x=xpos;
            last_cursor_y=ypos;
        }

        zoom=zoom<=10.f ? zoom : 10.f;

        // Rendering
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        {
            glUseProgram(pattern_shader);
            glBindVertexArray(vert_array);
            glBindBuffer(GL_ARRAY_BUFFER,vertexbuffers[0]);
            glEnableVertexAttribArray(attrib_pos);
            glEnableVertexAttribArray(attrib_uv);
            glVertexAttribPointer(attrib_pos, 2, GL_FLOAT, GL_FALSE,
                0, (void*)0 );
            glVertexAttribPointer(attrib_uv,  2, GL_FLOAT, GL_FALSE,
                0, (void*)0);
            glUniform1f(attrib_w,(float)param->pattern_width);
            glUniform1f(attrib_h,(float)param->pattern_height);
            glUniform1i(attrib_tex, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,pattern_tex);
            if(data.tile){
                glUniform1f(attrib_alpha,0.75f);
                for(int y=-1;y<=1;y++){
                    for(int x=-1;x<=1;x++){
                        get_projection_matrix(projection,display_w,display_h,
                            panel_width,image_x+x,image_y+y,zoom);
                        glUniformMatrix4fv(attrib_matrix, 1, GL_FALSE, projection);
                        glDrawArrays(GL_TRIANGLES,0,6);
                    }
                }
            }
            glUniform1f(attrib_alpha,1.0f);
            get_projection_matrix(projection,display_w,display_h,
                panel_width,image_x,image_y,zoom);
            glUniformMatrix4fv(attrib_matrix, 1, GL_FALSE, projection);
            glDrawArrays(GL_TRIANGLES,0,6);
        }
        ImGui::Render();

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();
    return param;
}

static void pattern_editor_set_imgui_style(){
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.10f, 0.10f, 0.10f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    style.Colors[ImGuiCol_Column]                = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.49f, 0.49f, 0.49f, 0.47f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.66f, 0.66f, 0.66f, 0.47f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.70f, 0.70f, 0.70f, 0.90f);
    style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);
    style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    style.WindowRounding=0.f;
    style.FrameRounding=3.f;
}

#endif

