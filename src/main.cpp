#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr int kWidth = 1000;
constexpr int kHeight = 700;
constexpr float kPi = 3.14159265359f;

float gCameraYaw = 0.0f;
const float gCameraDistance = 10.0f;
float gTimeScale = 1.0f;
}

struct SphereMesh {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    const float yawSpeed = 1.4f;  // radians per second
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        gCameraYaw -= yawSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        gCameraYaw += yawSpeed * deltaTime;
    }

    // Use multiplicative scaling so speed approaches zero but never flips direction.
    const float speedAdjustRate = 1.2f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        gTimeScale *= (1.0f + speedAdjustRate * deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        gTimeScale *= (1.0f - speedAdjustRate * deltaTime);
    }
    if (gTimeScale < 0.05f) {
        gTimeScale = 0.05f;
    }
    if (gTimeScale > 5.0f) {
        gTimeScale = 5.0f;
    }
}

unsigned int compileShader(unsigned int shaderType, const std::string& source) {
    const char* sourcePtr = source.c_str();
    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << "\n";
    }
    return shader;
}

unsigned int createProgram(const std::string& vertexSource, const std::string& fragmentSource) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Program linking failed:\n" << infoLog << "\n";
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

SphereMesh generateSphere(unsigned int stacks, unsigned int sectors) {
    SphereMesh mesh;
    mesh.vertices.reserve((stacks + 1) * (sectors + 1) * 6);
    mesh.indices.reserve(stacks * sectors * 6);

    for (unsigned int i = 0; i <= stacks; ++i) {
        const float stackAngle = kPi * 0.5f - static_cast<float>(i) * (kPi / static_cast<float>(stacks));
        const float xy = std::cos(stackAngle);
        const float z = std::sin(stackAngle);

        for (unsigned int j = 0; j <= sectors; ++j) {
            const float sectorAngle = static_cast<float>(j) * (2.0f * kPi / static_cast<float>(sectors));
            const float x = xy * std::cos(sectorAngle);
            const float y = xy * std::sin(sectorAngle);

            mesh.vertices.push_back(x);
            mesh.vertices.push_back(y);
            mesh.vertices.push_back(z);

            mesh.vertices.push_back(x);
            mesh.vertices.push_back(y);
            mesh.vertices.push_back(z);
        }
    }

    for (unsigned int i = 0; i < stacks; ++i) {
        const unsigned int k1 = i * (sectors + 1);
        const unsigned int k2 = k1 + sectors + 1;

        for (unsigned int j = 0; j < sectors; ++j) {
            if (i != 0) {
                mesh.indices.push_back(k1 + j);
                mesh.indices.push_back(k2 + j);
                mesh.indices.push_back(k1 + j + 1);
            }
            if (i != (stacks - 1)) {
                mesh.indices.push_back(k1 + j + 1);
                mesh.indices.push_back(k2 + j);
                mesh.indices.push_back(k2 + j + 1);
            }
        }
    }

    return mesh;
}

std::vector<float> generateOrbitCircle(unsigned int segments) {
    std::vector<float> vertices;
    vertices.reserve((segments + 1) * 3);
    for (unsigned int i = 0; i < segments; ++i) {
        float angle = (2.0f * kPi * static_cast<float>(i)) / static_cast<float>(segments);
        vertices.push_back(std::cos(angle));
        vertices.push_back(0.0f);
        vertices.push_back(std::sin(angle));
    }
    return vertices;
}

std::vector<float> generateStars(unsigned int count, float radius) {
    std::vector<float> stars;
    stars.reserve(count * 3);

    for (unsigned int i = 0; i < count; ++i) {
        const float u = std::fmod(std::sin(static_cast<float>(i) * 12.9898f) * 43758.5453f, 1.0f);
        const float v = std::fmod(std::sin(static_cast<float>(i) * 78.233f) * 12345.6789f, 1.0f);
        const float w = std::fmod(std::sin(static_cast<float>(i) * 45.164f) * 34567.1234f, 1.0f);

        const float azimuth = (u < 0.0f ? u + 1.0f : u) * 2.0f * kPi;
        const float z = ((v < 0.0f ? v + 1.0f : v) * 2.0f - 1.0f) * radius;
        const float ring = std::sqrt(radius * radius - z * z);

        const float x = ring * std::cos(azimuth);
        const float y = ring * std::sin(azimuth);
        const float scale = 0.92f + 0.08f * (w < 0.0f ? w + 1.0f : w);

        stars.push_back(x * scale);
        stars.push_back(y * scale);
        stars.push_back(z * scale);
    }

    return stars;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "Sun-Earth-Moon System", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    const std::string vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        out vec3 FragPos;
        out vec3 Normal;
        out vec3 LocalPos;
        out vec3 LocalNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            LocalPos = aPos;
            LocalNormal = aNormal;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    const std::string fragmentShaderSource = R"(
        #version 330 core
        in vec3 FragPos;
        in vec3 Normal;
        in vec3 LocalPos;
        in vec3 LocalNormal;

        out vec4 FragColor;

        uniform vec3 objectColor;
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform float emissiveStrength;
        uniform int bodyType;

        float hash31(vec3 p) {
            p = fract(p * 0.1031);
            p += dot(p, p.yzx + 33.33);
            return fract((p.x + p.y) * p.z);
        }

        float valueNoise3(vec3 p) {
            vec3 i = floor(p);
            vec3 f = fract(p);
            f = f * f * (3.0 - 2.0 * f);

            float n000 = hash31(i + vec3(0.0, 0.0, 0.0));
            float n100 = hash31(i + vec3(1.0, 0.0, 0.0));
            float n010 = hash31(i + vec3(0.0, 1.0, 0.0));
            float n110 = hash31(i + vec3(1.0, 1.0, 0.0));
            float n001 = hash31(i + vec3(0.0, 0.0, 1.0));
            float n101 = hash31(i + vec3(1.0, 0.0, 1.0));
            float n011 = hash31(i + vec3(0.0, 1.0, 1.0));
            float n111 = hash31(i + vec3(1.0, 1.0, 1.0));

            float nx00 = mix(n000, n100, f.x);
            float nx10 = mix(n010, n110, f.x);
            float nx01 = mix(n001, n101, f.x);
            float nx11 = mix(n011, n111, f.x);
            float nxy0 = mix(nx00, nx10, f.y);
            float nxy1 = mix(nx01, nx11, f.y);
            return mix(nxy0, nxy1, f.z);
        }

        vec3 proceduralAlbedo(vec3 baseColor, vec3 localNormal, vec3 localPos, vec3 fragPos) {
            vec3 n = normalize(localNormal);
            if (bodyType == 0) {
                // Sun: warped multi-direction pattern for less uniform stripes.
                vec3 p = localPos * 6.0 + vec3(fragPos.y * 0.2, fragPos.z * 0.2, fragPos.x * 0.2);
                vec3 warp = vec3(
                    valueNoise3(p + vec3(1.3, 2.1, 0.7)),
                    valueNoise3(p + vec3(3.7, 0.4, 2.5)),
                    valueNoise3(p + vec3(2.2, 4.6, 1.9))
                ) - 0.5;
                vec3 q = p + warp * 2.4;

                float bandA = 0.5 + 0.5 * sin(q.x * 2.0 + q.y * 1.6);
                float bandB = 0.5 + 0.5 * sin(q.z * 2.4 - q.x * 1.2 + 1.1);
                float cellular = valueNoise3(q * 0.9);
                float turbulence = valueNoise3(q * 1.8 + vec3(5.0, 1.0, 3.0));

                float mixedPattern = 0.35 * bandA + 0.35 * bandB + 0.3 * cellular;
                float glowTint = 0.75 + 0.45 * mixedPattern + 0.2 * turbulence;
                float darkPatches = smoothstep(0.58, 0.86, valueNoise3(q * 1.35 + vec3(8.0, 2.0, 6.0)));

                vec3 deepRed = vec3(0.88, 0.18, 0.05);
                vec3 hotOrange = vec3(1.0, 0.42, 0.08);
                vec3 brightCore = vec3(1.0, 0.70, 0.10);

                vec3 lava = mix(deepRed, hotOrange, mixedPattern);
                lava = mix(lava, brightCore, pow(max(mixedPattern, 0.0), 2.2));
                lava = mix(lava, deepRed * 0.9, darkPatches * 0.45);
                return lava * glowTint;
            } else if (bodyType == 1) {
                // Earth: pseudo continents/oceans with slight cloud tint.
                float continents = valueNoise3(localPos * 4.5 + vec3(2.0, 1.0, 3.0));
                float coast = smoothstep(0.46, 0.58, continents);
                vec3 ocean = vec3(0.08, 0.28, 0.72);
                vec3 land = vec3(0.16, 0.56, 0.22);
                vec3 earth = mix(ocean, land, coast);
                float clouds = smoothstep(0.68, 0.85, valueNoise3(localPos * 11.0 + vec3(7.0)));
                return mix(earth, vec3(0.9, 0.93, 0.95), clouds * 0.35);
            }

            // Moon: grayscale rocky noise with crater-like contrast.
            float rocky = valueNoise3(localPos * 7.5 + vec3(5.0, 2.0, 4.0));
            float craters = valueNoise3(localPos * 18.0 + vec3(11.0, 3.0, 9.0));
            float shade = 0.55 + rocky * 0.35 - craters * 0.2;
            return vec3(shade);
        }

        void main() {
            vec3 albedo = proceduralAlbedo(objectColor, LocalNormal, LocalPos, FragPos);
            vec3 ambient = 0.15 * albedo;

            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diffStrength = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diffStrength * albedo;

            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float specStrength = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
            vec3 specular = vec3(0.6) * specStrength;

            vec3 emissive = emissiveStrength * albedo;
            vec3 color = ambient + diffuse + specular + emissive;
            FragColor = vec4(color, 1.0);
        }
    )";

    const unsigned int shaderProgram = createProgram(vertexShaderSource, fragmentShaderSource);
    const std::string orbitVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";
    const std::string orbitFragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 lineColor;
        void main() {
            FragColor = vec4(lineColor, 1.0);
        }
    )";
    const unsigned int orbitShaderProgram = createProgram(orbitVertexShaderSource, orbitFragmentShaderSource);
    const std::string starVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
            gl_PointSize = 2.2;
        }
    )";
    const std::string starFragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.95, 0.95, 1.0, 1.0);
        }
    )";
    const unsigned int starShaderProgram = createProgram(starVertexShaderSource, starFragmentShaderSource);

    SphereMesh sphere = generateSphere(32, 32);

    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sphere.vertices.size() * sizeof(float)), sphere.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(sphere.indices.size() * sizeof(unsigned int)),
        sphere.indices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    std::vector<float> orbitVertices = generateOrbitCircle(160);
    unsigned int orbitVao = 0;
    unsigned int orbitVbo = 0;
    glGenVertexArrays(1, &orbitVao);
    glGenBuffers(1, &orbitVbo);
    glBindVertexArray(orbitVao);
    glBindBuffer(GL_ARRAY_BUFFER, orbitVbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(orbitVertices.size() * sizeof(float)),
        orbitVertices.data(),
        GL_STATIC_DRAW
    );
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    std::vector<float> starVertices = generateStars(900, 55.0f);
    unsigned int starVao = 0;
    unsigned int starVbo = 0;
    glGenVertexArrays(1, &starVao);
    glGenBuffers(1, &starVbo);
    glBindVertexArray(starVao);
    glBindBuffer(GL_ARRAY_BUFFER, starVbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(starVertices.size() * sizeof(float)),
        starVertices.data(),
        GL_STATIC_DRAW
    );
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    float lastFrameTime = static_cast<float>(glfwGetTime());
    float simulationTime = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        const float currentTime = static_cast<float>(glfwGetTime());
        const float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        processInput(window, deltaTime);
        simulationTime += deltaTime * gTimeScale;

        glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
        const glm::vec3 cameraPos(
            std::sin(gCameraYaw) * gCameraDistance,
            3.5f,
            std::cos(gCameraYaw) * gCameraDistance
        );

        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(kWidth) / static_cast<float>(kHeight), 0.1f, 100.0f);

        glUseProgram(starShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(starShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(starShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(starVao);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(starVertices.size() / 3));

        const float earthOrbitRadius = 4.0f;
        const float moonOrbitRadius = 1.2f;
        const float earthOrbitSpeed = 0.8f;
        const float moonOrbitSpeed = 2.2f;
        const float earthSpinSpeed = 1.8f;
        const float moonSpinSpeed = 1.2f;
        const float earthOrbitInclination = glm::radians(30.0f);
        const float moonOrbitInclination = glm::radians(16.0f);

        // Tilt around Z to create a clear right-leaning orbit in screen view.
        const glm::mat4 earthOrbitTilt = glm::rotate(glm::mat4(1.0f), earthOrbitInclination, glm::vec3(0.0f, 0.0f, 1.0f));
        const glm::mat4 moonOrbitTilt = glm::rotate(glm::mat4(1.0f), moonOrbitInclination, glm::normalize(glm::vec3(0.4f, 0.0f, 1.0f)));

        const glm::vec3 earthLocalPos(
            std::cos(simulationTime * earthOrbitSpeed) * earthOrbitRadius,
            0.0f,
            std::sin(simulationTime * earthOrbitSpeed) * earthOrbitRadius
        );
        const glm::vec3 earthPos = glm::vec3(earthOrbitTilt * glm::vec4(earthLocalPos, 1.0f));

        const glm::vec3 moonLocalPos(
            std::cos(simulationTime * moonOrbitSpeed) * moonOrbitRadius,
            0.0f,
            std::sin(simulationTime * moonOrbitSpeed) * moonOrbitRadius
        );
        const glm::vec3 moonOffset = glm::vec3(moonOrbitTilt * glm::vec4(moonLocalPos, 1.0f));
        const glm::vec3 moonPos = earthPos + moonOffset;

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));

        glUseProgram(orbitShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(orbitShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(orbitShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(orbitVao);

        glm::mat4 earthOrbitModel = earthOrbitTilt;
        earthOrbitModel = glm::scale(earthOrbitModel, glm::vec3(earthOrbitRadius));
        glUniformMatrix4fv(glGetUniformLocation(orbitShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(earthOrbitModel));
        glUniform3f(glGetUniformLocation(orbitShaderProgram, "lineColor"), 0.35f, 0.35f, 0.45f);
        glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(orbitVertices.size() / 3));

        glm::mat4 moonOrbitModel = glm::translate(glm::mat4(1.0f), earthPos) * moonOrbitTilt;
        moonOrbitModel = glm::scale(moonOrbitModel, glm::vec3(moonOrbitRadius));
        glUniformMatrix4fv(glGetUniformLocation(orbitShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(moonOrbitModel));
        glUniform3f(glGetUniformLocation(orbitShaderProgram, "lineColor"), 0.25f, 0.25f, 0.3f);
        glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(orbitVertices.size() / 3));

        glUseProgram(shaderProgram);
        glBindVertexArray(vao);

        glm::mat4 sunModel = glm::scale(glm::mat4(1.0f), glm::vec3(1.2f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(sunModel));
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 0.30f, 0.08f);
        glUniform1f(glGetUniformLocation(shaderProgram, "emissiveStrength"), 1.55f);
        glUniform1i(glGetUniformLocation(shaderProgram, "bodyType"), 0);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sphere.indices.size()), GL_UNSIGNED_INT, nullptr);

        glm::mat4 earthModel = glm::translate(glm::mat4(1.0f), earthPos);
        earthModel = glm::rotate(earthModel, simulationTime * earthSpinSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
        earthModel = glm::scale(earthModel, glm::vec3(0.5f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(earthModel));
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.2f, 0.5f, 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "emissiveStrength"), 0.0f);
        glUniform1i(glGetUniformLocation(shaderProgram, "bodyType"), 1);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sphere.indices.size()), GL_UNSIGNED_INT, nullptr);

        glm::mat4 moonModel = glm::translate(glm::mat4(1.0f), moonPos);
        moonModel = glm::rotate(moonModel, simulationTime * moonSpinSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
        moonModel = glm::scale(moonModel, glm::vec3(0.2f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(moonModel));
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.8f, 0.8f, 0.85f);
        glUniform1f(glGetUniformLocation(shaderProgram, "emissiveStrength"), 0.0f);
        glUniform1i(glGetUniformLocation(shaderProgram, "bodyType"), 2);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sphere.indices.size()), GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteVertexArrays(1, &orbitVao);
    glDeleteVertexArrays(1, &starVao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &orbitVbo);
    glDeleteBuffers(1, &starVbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(orbitShaderProgram);
    glDeleteProgram(starShaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}