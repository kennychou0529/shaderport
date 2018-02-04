#pragma once

void GLErrorCodeString(GLenum error)
{
         if (error == GL_INVALID_ENUM)                  return "enum argument out of range\n";
    else if (error == GL_INVALID_VALUE)                 return "Numeric argument out of range\n";
    else if (error == GL_INVALID_OPERATION)             return "Operation illegal in current state\n";
    else if (error == GL_INVALID_FRAMEBUFFER_OPERATION) return "Framebuffer object is not complete\n";
    else if (error == GL_OUT_OF_MEMORY)                 return "Not enough memory left to execute command (or the driver implementation didn't bother with a proper error code)\n";
    return "Not an error";
}

void CheckGLError()
{
    GLenum error = glGetError();
    while (error != GL_NO_ERROR)
    {
        printf("OpenGL error: (0x%x) %s", error, GLErrorCodeString(error));
        error = glGetError();
    }
}
