#include <GL/freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION  // �ݵ�� stbi_load �Լ� ���� ���� �߰�
#include "stb_image.h"


void rotateAroundAxis(float& vx, float& vy, float& vz, float ax, float ay, float az, float angle);
float cameraX = 0.0f, cameraY = 1.0f, cameraZ = 5.0f; // �ʱ� ī�޶� ��ġ
float cameraTargetX = 0.0f, cameraTargetY = 1.0f, cameraTargetZ = 0.0f; // ī�޶� ���� ����
float upX = 0.0f, upY = 1.0f, upZ = 0.0f; // ���� ����
float directionX = 0.0f, directionY = 0.0f, directionZ = -1.0f; // ī�޶� ����

struct Missile {
    bool active = false;
    float Xeye, Yeye, Zeye; 
    float Nx, Ny, Nz;       
    float speed = 1.0f;     
    float length = 2.0f;    
    float radius = 0.2f;   
} missile;


float pitchAngle = 0.0f; // ���Ʒ� ȸ�� ����
float yawAngle = 0.0f;   // �¿� ȸ�� ����
float rollAngle = 0.0f;  // �¿� ����

float cameraDirectionX = 0.0f; // X ����
float cameraDirectionY = 0.0f; // Y ����
float cameraDirectionZ = -1.0f; // Z ����

#define PI 3.141592
#define BMP_Header_Length 54


float terrain[527][527]; // ���� ������
GLuint terrainTexture; // �ؽ�ó ID
GLuint skyboxTextures[6];


float normalPositionX;
float normalPositionY;
float normalPositionZ;



float lakeHeight = 50.0f; // ȣ���� ����
GLuint lakeTexture;       // ȣ�� �ؽ�ó



// ���� ũ��� �ؽ�ó
int terrainWidth = 527;
int terrainHeight = 527;
// �浹 �ݰ�
float collisionRadius = 0.5f;

bool isGameOver = false;



typedef struct {
    float x, y, z;
} Vertex;

typedef struct {
    int v1, v2, v3; // �ﰢ�� �ε���
} Face;

Vertex* VaseVertices = NULL;
Face* VaseFaces = NULL;
int vertexCount = 0, faceCount = 0;
int VaseFaceCount = 0;

// OBJ ���� �б�
void loadOBJ(const char* filename, Vertex** vertices, Face** faces) {
    printf("�ε� ���� ����: %s\n", filename);
    FILE* file;
    fopen_s(&file, filename, "r");           ///obj���� �ҷ�����
    if (!file) {
        printf("������ �� �� �����ϴ�: %s\n", filename);
        exit(1);
    }

    // �ӽ� ī��Ʈ
    int tempVertexCount = 0, tempFaceCount = 0;
    char line[128];

    // ù ��° �н�: ī��Ʈ ���
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') tempVertexCount++;                ///v(����)�� �ν��Ͽ� ����� ����
        else if (line[0] == 'f' && line[1] == ' ') tempFaceCount++;                ///f(��)�� �ν��Ͽ� ��� ������ ����
    }

    // �޸� �Ҵ�
    *vertices = (Vertex*)malloc(sizeof(Vertex) * tempVertexCount);           //���� ����ŭ �������� �޸� �Ҵ�
    *faces = (Face*)malloc(sizeof(Face) * tempFaceCount * 2); // �簢���� ������ �� 2��� �÷��� ��

    // ���� �ٽ� �б�
    fseek(file, 0, SEEK_SET);          ///������ �ٽ� �о�
    int vertexIndex = 0, faceIndex = 0;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            // Vertex �б�
            sscanf_s(line, "v %f %f %f", &(*vertices)[vertexIndex].x, &(*vertices)[vertexIndex].y, &(*vertices)[vertexIndex].z);    ///������ ���� v �� �ִ°��� ������ �����Ѵ�.
            
            vertexIndex++;
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            int v1, v2, v3, v4;
            int t1, t2, t3, t4;

            if (sscanf_s(line, "f %d/%d %d/%d %d/%d %d/%d", &v1, &t1, &v2, &t2, &v3, &t3, &v4, &t4) == 8) {    ///�� �����͸� ����
                // �簢���� �� �ﰢ������ ����
                (*faces)[faceIndex].v1 = v1; (*faces)[faceIndex].v2 = v2; (*faces)[faceIndex].v3 = v3;
                faceIndex++;
                (*faces)[faceIndex].v1 = v1; (*faces)[faceIndex].v2 = v3; (*faces)[faceIndex].v3 = v4;
                faceIndex++;
            }
            else if (sscanf_s(line, "f %d/%d %d/%d %d/%d", &v1, &t1, &v2, &t2, &v3, &t3) == 6) {
                // �ﰢ�� ó��
                (*faces)[faceIndex].v1 = v1; (*faces)[faceIndex].v2 = v2; (*faces)[faceIndex].v3 = v3;
                faceIndex++;
            }
        }
    }

    vertexCount = tempVertexCount;
    VaseFaceCount = faceIndex;  // faceIndex�� ���� ó���� ��ü ���� ��
    fclose(file);

}

void renderVaseModel() {
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < VaseFaceCount; i++) {
        Vertex v1 = VaseVertices[VaseFaces[i].v1 - 1];
        Vertex v2 = VaseVertices[VaseFaces[i].v2 - 1];
        Vertex v3 = VaseVertices[VaseFaces[i].v3 - 1];

        //printf("Rendering Vertex: (%f, %f, %f)\n", v1.x, v1.y, v1.z);  // �� ���� ���

        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v2.x, v2.y, v2.z);
        glVertex3f(v3.x, v3.y, v3.z);
    }

    glEnd();
}


//////////////////////////////////////////////////////////////////////// ���� ����

bool checkCollision(float playerX, float playerZ, float playerY) {
    int gridX = (playerX)+284;
    int gridZ = (playerZ)+284;

    // ������ ��ȿ ���� Ȯ��

    // ���� ��ġ�� ���� ���� ��������
    float terrainHeightAtPlayer = terrain[gridX][gridZ] - 80.0f;
    //std::cout << terrainHeightAtPlayer << std::endl;
    // �÷��̾ ���� ���̸� �ʰ��ϸ� �浹
    if (playerY <= terrainHeightAtPlayer + collisionRadius) {
        return true;
    }

    return false;
}

// ��ƼŬ ����ü ����
struct Particle {
    float x, y, z;        // ��ġ
    float vx, vy, vz;     // �ӵ�
    float lifetime;       // ���� ����
};
std::vector<Particle> particles;

void initParticle(Particle& p, float startX, float startY, float startZ) {
    p.x = startX;
    p.y = startY;
    p.z = startZ;
    p.vx = ((rand() % 100) / 50.0f - 1.0f) * 2.0f; 
    p.vy = ((rand() % 100) / 50.0f) * 5.0f;        
    p.vz = ((rand() % 100) / 50.0f - 1.0f) * 2.0f; 
    p.lifetime = 3.0f;                             // �ʱ� ����
    
}
Particle p;
// ��ƼŬ ���� �Լ�
void createParticleEffect(float x, float y, float z) {
    for (int i = 0; i < 10; ++i) { // ��ƼŬ 10�� ����

        initParticle(p, x, y, z);
        particles.push_back(p);
    }
    std::cout << "��ƼŬ ���� ����: " << particles.size() << std::endl;
}
// ��ƼŬ ������Ʈ �Լ�
void updateParticles(float deltaTime) {
    for (auto it = particles.begin(); it != particles.end();) {
        it->x += it->vx * deltaTime;
        it->y += it->vy * deltaTime;
        it->z += it->vz * deltaTime;
        it->vy -= 9.8f * deltaTime; 
        it->lifetime -= deltaTime;

        // ������ ���� ��ƼŬ ����
        if (it->lifetime <= 0.0f) {
            it = particles.erase(it);
            missile.active = false;
        }
        else {
            ++it;
        }
       // std::cout << p.lifetime << std::endl;
    }
}

// �� ������ �Լ�
void drawSphere(float x, float y, float z, float radius) {
    glPushMatrix(); 
    glTranslatef(x, y, z); 
    GLUquadric* quad = gluNewQuadric(); 
    gluSphere(quad, radius, 16, 16); 
    gluDeleteQuadric(quad); 
    glPopMatrix(); 
}

// ��ƼŬ ������ �Լ�
void renderParticles() {
    
    for (const auto& p : particles) {
        float alpha = p.lifetime; // ���� ��� ���� ����
        glColor4f(1.0f, 0.3f, 0.0f, alpha); // ��Ȳ�� ��ƼŬ
        drawSphere(p.x, p.y, p.z, 0.9f); // ������ 0.1f�� �� �׸���
        std::cout << "��ƼŬ ���� ��ġ: " << p.x << p.y << p.z << std::endl;
    }


}

// �̻��� �浹 �˻� �Լ� (���� ���)
bool checkMissileCollision(float missileX, float missileZ, float missileY) {
    // ������ ��ȿ ���� Ȯ��
    int gridX = missileX + 284; // ������ x ��ǥ
    int gridZ = missileZ + 284; // ������ z ��ǥ

    // ���� ��ġ�� ���� ���� ��������
    float terrainHeightAtMissile = terrain[gridX][gridZ] - 80.0f;

    // �̻����� ���� ���̺��� ���ų� ������ �浹�� ����
    if (missileY <= terrainHeightAtMissile) {
        createParticleEffect(missileX, terrainHeightAtMissile, missileZ);

        return true;
    }

    return false;
}


bool loadHeightmap(const char* filepath) {
    int width, height, channels;
    unsigned char* data = stbi_load(filepath, &width, &height, &channels, 1);

    if (data == nullptr) {
        std::cerr << "Failed to load heightmap. Path: " << filepath << std::endl;
        return false;
    }
    if (channels != 1) {
        std::cerr << "Heightmap must be grayscale!" << std::endl;
        stbi_image_free(data);
        return false;
    }

    // heightmap�� �ȼ� ���� terrain �迭�� �Ҵ�
    if (width > 527 || height > 527) {
        std::cerr << "Heightmap size exceeds terrain array limits!" << std::endl;
        stbi_image_free(data);
        return false;
    }

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            terrain[i][j] = (float)data[i * width + j]; // ���̸� 0.0~1.0���� ����ȭ
        }
    }

    stbi_image_free(data);
    return true;
}


void loadBMPTexture(const char* filename, GLuint* textureID) {
    unsigned char header[BMP_Header_Length];
    unsigned char* data;
    unsigned int width, height;
    unsigned int dataPos, imageSize;

    FILE* file = NULL;
    errno_t err = fopen_s(&file, filename, "rb");  ///BMP������ �о� �´�
    if (err != 0) {
        printf("������ �� �� �����ϴ�: %s\n", filename);     //���� �� ���� �� 
        exit(1);
    }

    fread(header, 1, BMP_Header_Length, file);      ///BMP ������ �⺻ ������ 54����Ʈ ũ���� ����� �����Ѵ�.
    width = *(int*)&header[18];                       
    height = *(int*)&header[22];               ///�̹����� ���� ���� ������ ����
    dataPos = *(int*)&header[10];                     ///�̹����� ���� ��ġ
    imageSize = width * height * 3;       ///�̹����� ũ�� ���  (ä�� ����)

    data = (unsigned char*)malloc(imageSize);      ///�̹��� �����͸� ������ �޸𸮸� �������� �Ҵ�
    fseek(file, dataPos, SEEK_SET);      ///�̹��� �������� ���� ��ġ�� ���� ������ �̵�
    fread(data, 1, imageSize, file);      ///�̹��� �����͸� ���Ͽ��� �޸𸮷� �ϰſ´�.
    fclose(file);                                     //���� �ݱ�

    glGenTextures(1, textureID);      ///������ ���� �ؽ�ó(�̹���) �����ϰ� ID �� ��ȯ�Ѵ�
    glBindTexture(GL_TEXTURE_2D, *textureID);                 //������ �ؽ�ó�� Ȱ��ȭ�ϰ� ���� �۾��� �� �ؽ�ó�� �����ϰ� �Ѵ�.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);           ///�̹����� ũ�⿡ �°� �ؽ�ó�� 2d �̹����� �����Ѵ�.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);          ////�ؽ�ó�� Ȯ�� ����� ���� ���͸�
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    free(data);
}

void loadBMPTexture(const char* filename) {
    //FILE* file;
    unsigned char header[BMP_Header_Length];
    unsigned char* data;
    unsigned int width, height;
    unsigned int dataPos, imageSize;

    FILE* file = NULL;
    errno_t err = fopen_s(&file, filename, "rb");
    if (err != 0) {
        printf("������ �� �� �����ϴ�: %s\n", filename);
        exit(1);
    }
    if (!file) {
        printf("BMP ������ �� �� �����ϴ�: %s\n", filename);
        exit(1);
    }

    fread(header, 1, BMP_Header_Length, file);
    width = *(int*)&header[18];
    height = *(int*)&header[22];
    dataPos = *(int*)&header[10];
    imageSize = width * height * 3;

    data = (unsigned char*)malloc(imageSize);
    fseek(file, dataPos, SEEK_SET);
    fread(data, 1, imageSize, file);
    fclose(file);

    glGenTextures(1, &terrainTexture);
    glBindTexture(GL_TEXTURE_2D, terrainTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    free(data);
}

void loadSkyboxTextures() {
    loadBMPTexture("front (1).bmp", &skyboxTextures[0]);
    loadBMPTexture("back (1).bmp", &skyboxTextures[1]);
    loadBMPTexture("right (1).bmp", &skyboxTextures[2]);
    loadBMPTexture("left (1).bmp", &skyboxTextures[3]);
    loadBMPTexture("top (1).bmp", &skyboxTextures[4]);
    loadBMPTexture("bottom (1).bmp", &skyboxTextures[5]);
}


void drawTerrain(int terrainWidth, int terrainHeight) {
    glBindTexture(GL_TEXTURE_2D, terrainTexture);      ///���� �ؽ�ó
    glBegin(GL_QUADS);

    for (int x = 0; x < terrainWidth - 1; ++x) {
        for (int z = 0; z < terrainHeight - 1; ++z) {
            // �ؽ�ó ��ǥ�� ���� ũ�⿡ ���� ����ȭ
            float texCoordX = (float)x / (float)(terrainWidth - 1);         ////�ؽ�ó�� ���� ũ�⿡ ���缭 ����
            float texCoordZ = (float)z / (float)(terrainHeight - 1);    

            glTexCoord2f(texCoordX, texCoordZ);               glVertex3f(x, terrain[x][z], z);          ///(x, y)�� (x+1, y+1) ������ �簡������ ���� ����                          
            glTexCoord2f(texCoordX + 1.0f / terrainWidth, texCoordZ); glVertex3f(x + 1, terrain[x + 1][z], z);
            glTexCoord2f(texCoordX + 1.0f / terrainWidth, texCoordZ + 1.0f / terrainHeight); glVertex3f(x + 1, terrain[x + 1][z + 1], z + 1);
            glTexCoord2f(texCoordX, texCoordZ + 1.0f / terrainHeight); glVertex3f(x, terrain[x][z + 1], z + 1);
        }
    }

    glEnd();
}

void drawLake(int terrainWidth, int terrainHeight) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ���� ����

    glBindTexture(GL_TEXTURE_2D, lakeTexture);
    glBegin(GL_QUADS);

    for (int x = 0; x < terrainWidth - 1; ++x) {
        for (int z = 0; z < terrainHeight - 1; ++z) {
            if (terrain[x][z] <= lakeHeight) { // ȣ�� ����
                float texCoordX = (float)x / (float)(terrainWidth - 1);
                float texCoordZ = (float)z / (float)(terrainHeight - 1);

                glColor4f(0.0f, 0.5f, 1.0f, 0.6f); // �������� �Ķ���

                glTexCoord2f(texCoordX, texCoordZ);
                glVertex3f(x, lakeHeight, z);

                glTexCoord2f(texCoordX + 1.0f / terrainWidth, texCoordZ);
                glVertex3f(x + 1, lakeHeight, z);

                glTexCoord2f(texCoordX + 1.0f / terrainWidth, texCoordZ + 1.0f / terrainHeight);
                glVertex3f(x + 1, lakeHeight, z + 1);

                glTexCoord2f(texCoordX, texCoordZ + 1.0f / terrainHeight);
                glVertex3f(x, lakeHeight, z + 1);

                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
    }

    glEnd();
    glDisable(GL_BLEND);
}


void drawSkybox() {
    glPushMatrix();

    glEnable(GL_DEPTH_TEST); // ��ī�̹ڽ��� �ڿ� ���̵���

    // Front
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(500.0f, -500.0f, -500.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(500.0f, 500.0f, -500.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-500.0f, 500.0f, -500.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-500.0f, -500.0f, -500.0f);
    glEnd();

    // Back
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-500.0f, -500.0f, 500.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(500.0f, -500.0f, 500.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(500.0f, 500.0f, 500.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-500.0f, 500.0f, 500.0f);
    glEnd();

    // Left
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[2]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-500.0f, -500.0f, -500.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-500.0f, -500.0f, 500.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-500.0f, 500.0f, 500.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-500.0f, 500.0f, -500.0f);
    glEnd();

    // Right
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[3]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(500.0f, -500.0f, 500.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(500.0f, -500.0f, -500.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(500.0f, 500.0f, -500.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(500.0f, 500.0f, 500.0f);
    glEnd();

    // Top
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[4]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(500.0f, 500.0f, -500.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(500.0f, 500.0f, 500.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-500.0f, 500.0f, 500.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-500.0f, 500.0f, -500.0f);
    glEnd();

    // Bottom
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[5]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(500.0f, -500.0f, 500.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-500.0f, -500.0f, 500.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-500.0f, -500.0f, -500.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(500.0f, -500.0f, -500.0f);
    glEnd();

    glEnable(GL_DEPTH_TEST); // �ٽ� depth test Ȱ��ȭ

    glPopMatrix();
}


void drawCylinder(float radius, float length) {
    int segments = 20; // Number of segments to approximate the circle
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * PI * i / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);

        // Bottom circle
        glVertex3f(x, y, 0.0f);

        // Top circle
        glVertex3f(x, y, length);
    }
    glEnd();

    // Create top cap
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.0f, length); // Center point of top
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * PI * i / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        glVertex3f(x, y, length);
    }
    glEnd();

    // Create bottom cap
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.0f, 0.0f); // Center point of bottom
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * PI * i / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        glVertex3f(x, y, 0.0f);
    }
    glEnd();
}



void renderObjectInFront() {
    glPushMatrix();

   
    float offset = 5.0f; 
    float objectX = cameraX + directionX * offset;       ///ī�޶� �ٶ󺸴� ���⿡ ������Ʈ ��ġ
    float objectY = cameraY + directionY * offset;
    float objectZ = cameraZ + directionZ * offset;

   
    glTranslatef(objectX, objectY, objectZ);

 
    float viewMatrix[16];
    
    glGetFloatv(GL_MODELVIEW_MATRIX, viewMatrix);   //�𵨺� ����� �����´�.

    // ī�޶� ȸ���� �ݿ��� ����ȯ ��� ����
    // ȸ���� ������ ���� (ī�޶��� ���� ���͸� ���ݴ�� ȸ��)
    GLfloat inverseRotationMatrix[16] = {
        viewMatrix[0], viewMatrix[4], viewMatrix[8],  0.0f,
        viewMatrix[1], viewMatrix[5], viewMatrix[9],  0.0f,
        viewMatrix[2], viewMatrix[6], viewMatrix[10], 0.0f,
        0.0f,         0.0f,         0.0f,           1.0f
    };
    glMultMatrixf(inverseRotationMatrix);

    
    glColor4f(0.2f, 0.3f, 0.2f, 1.0f);

    glRotatef(-13, 1.0f, 0.0f, 0.0f);
    glRotatef(180, 0.0f, 1.0f, 0.0f);
    glTranslatef(0.5f, -1.8f, -1.0f);

 

    // ũ�� ���� �� �߰� �� ������
    glScalef(0.001f, 0.001f, 0.001f);
    renderVaseModel();

    glPopMatrix();
}



void updateMissile() {
    if (missile.active) {
        // Update missile position based on direction and speed
        missile.Xeye += missile.Nx * missile.speed;
        missile.Yeye += missile.Ny * missile.speed;
        missile.Zeye += missile.Nz * missile.speed;

        // Calculate rotation based on missile direction and camera orientation
        GLfloat cameraDirection[] = { 0.0f, 0.0f, -1.0f }; // Assuming camera is at (0, 0, 0) and looking down -Z axis
        GLfloat missileDirection[] = { missile.Nx, missile.Ny, missile.Nz };

        // Normalize vectors
        GLfloat magnitudeCam = sqrt(cameraDirection[0] * cameraDirection[0] +
            cameraDirection[1] * cameraDirection[1] +
            cameraDirection[2] * cameraDirection[2]);

        GLfloat magnitudeMissile = sqrt(missileDirection[0] * missileDirection[0] +
            missileDirection[1] * missileDirection[1] +
            missileDirection[2] * missileDirection[2]);

        for (int i = 0; i < 3; i++) {
            cameraDirection[i] /= magnitudeCam;
            missileDirection[i] /= magnitudeMissile;
        }

        // Find the axis of rotation (cross product)
        GLfloat axis[3];
        axis[0] = cameraDirection[1] * missileDirection[2] - cameraDirection[2] * missileDirection[1];
        axis[1] = cameraDirection[2] * missileDirection[0] - cameraDirection[0] * missileDirection[2];
        axis[2] = cameraDirection[0] * missileDirection[1] - cameraDirection[1] * missileDirection[0];

        // Calculate the angle between vectors
        GLfloat dotProduct = cameraDirection[0] * missileDirection[0] +
            cameraDirection[1] * missileDirection[1] +
            cameraDirection[2] * missileDirection[2];

        GLfloat angle = acos(dotProduct) * 180.0 / PI;

        // Apply rotation
        glPushMatrix();
        glTranslatef(missile.Xeye, missile.Yeye, missile.Zeye );
        glTranslatef(0.0f, -3.0f, 0.0f);
        glRotatef(angle, axis[0], axis[1], axis[2]);
        glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        drawCylinder(missile.radius, missile.length);

        glPopMatrix();
    }
}

float distance = 1.0f;
int first = 0;
float camerawY = 0.0f;
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();


    // ī�޶� �ٶ󺸴� ��ġ ���
    //camera.updateViewMatrix();
    gluLookAt(
        cameraX, cameraY, cameraZ,           // ī�޶� ��ġ
        cameraTargetX, cameraTargetY, cameraTargetZ, // ī�޶� Ÿ��
        upX, upY, upZ                        // ���ŵ� Up ����
    );


    // �̻��� ������ �� �̵�
    if (missile.active) {
        glPushMatrix();
        updateMissile();
        glPopMatrix();

        if (!isGameOver && checkMissileCollision(missile.Xeye, missile.Zeye, missile.Yeye)) {
            std::cout << "����!" << std::endl;
            renderParticles();
            //missile.active = false;
            //first = 0;
        }
    }


    camerawY = cameraY;
    // �������� ���ο� ���� ��� ���� ����
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    GLfloat waterFogColor[4] = { 0.0, 0.3, 0.6, 3.0 };
    GLfloat fogColor[4] = { 0.75, 0.75, 0.75, 0.0 };
    if (camerawY < lakeHeight - 78) {
        glFogfv(GL_FOG_COLOR, waterFogColor);

        glFogf(GL_FOG_DENSITY, 0.075);
    }
    else {
        glFogfv(GL_FOG_COLOR, fogColor);

        glFogf(GL_FOG_DENSITY, 0.001);
    }

    if (!isGameOver && checkCollision(cameraX, cameraZ, cameraY)) {
        //isGameOver = true;
        std::cout << "���� ����!" << std::endl;
        exit(0);
    }



    // ī�޶� ��ġ�� �°� ����� �̵���Ŵ
    renderObjectInFront();




    glColor3f(1.0f, 1.0f, 1.0f);
    // ��ī�̹ڽ� ������
    glPushMatrix();
    glTranslatef(0.0f, -70.0f, 0.0);
    drawSkybox();
    glPopMatrix();

    // ������ ȣ�� ������
    glPushMatrix();

    //glScalef(2.0f, 2.0f, 2.0f);
    glTranslatef(-284.0f, 0.0f, -284.0f);
    glTranslatef(0.0f, -80.0f, 0.0f);
    drawTerrain(527, 527);
    drawLake(527, 527); // ȣ�� �׸���
    glPopMatrix();

    glutSwapBuffers();
}

float moveSpeedPerFrame = 0.2f; // �����Ӵ� �̵� �ӵ�
void moveForwardPerFrame() {
    // ���� �࿡ ���� �̵�
    cameraX += directionX * moveSpeedPerFrame;
    cameraY += directionY * moveSpeedPerFrame;
    cameraZ += directionZ * moveSpeedPerFrame;

    // Ÿ�ٵ� ������ �������� �̵�
    cameraTargetX += directionX * moveSpeedPerFrame;
    cameraTargetY += directionY * moveSpeedPerFrame;
    cameraTargetZ += directionZ * moveSpeedPerFrame;
}
void idle() {
    moveForwardPerFrame(); // �����Ӹ��� ī�޶� �̵�
    glutPostRedisplay();   // ȭ�� ����
}




void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 1.0f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

void rotateAroundAxis(float& vx, float& vy, float& vz, float ax, float ay, float az, float angle) {   ///�� ax, ay, az�� ������ �� ���� �������� vx, vy, vz�� ȸ��
    float rad = angle * 3.14159265f / 180.0f; // ������ �������� ��ȯ
    float cosAngle = cos(rad);
    float sinAngle = sin(rad);

    // �࿡ ���� ���� ȸ�� (�ε帮�Խ� ȸ�� ����)   �� ������ �̿��Ͽ� �ش� ���� �������� vx, vy, vz
    float dot = vx * ax + vy * ay + vz * az; // ���Ϳ� ���� ����
    float crossX = ay * vz - az * vy;        // ��� ������ ����
    float crossY = az * vx - ax * vz;
    float crossZ = ax * vy - ay * vx;

    vx = cosAngle * vx + (1 - cosAngle) * dot * ax + sinAngle * crossX;
    vy = cosAngle * vy + (1 - cosAngle) * dot * ay + sinAngle * crossY;
    vz = cosAngle * vz + (1 - cosAngle) * dot * az + sinAngle * crossZ;
}


float moveSpeed = 0.3f;
void handleKeyboard(unsigned char key, int x, int y) {
    float rotateSpeed = 2.0f;

    // Right ���� ��� (���� ���Ϳ� Up ������ ����)
    float rightX = upY * directionZ - upZ * directionY;
    float rightY = upZ * directionX - upX * directionZ;
    float rightZ = upX * directionY - upY * directionX;

    // ���� ����ȭ
    float rightLength = sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
    rightX /= rightLength;
    rightY /= rightLength;
    rightZ /= rightLength;

    if (key == 'w') {  // Pitch ���� (Right �� ����)
        rotateAroundAxis(directionX, directionY, directionZ, rightX, rightY, rightZ, rotateSpeed);             ///��� ���� x, y, z�� �����ϰ� ���⼭ �� ������ ���͸� ������ ȸ��
        rotateAroundAxis(upX, upY, upZ, rightX, rightY, rightZ, rotateSpeed);                                  ///���� ���� ����, �� ���ʹ� ������ ���� �������� ȸ��
        pitchAngle += rotateSpeed;
    }
    if (key == 's') {  // Pitch �Ʒ��� (Right �� ����)
        rotateAroundAxis(directionX, directionY, directionZ, rightX, rightY, rightZ, -rotateSpeed);
        rotateAroundAxis(upX, upY, upZ, rightX, rightY, rightZ, -rotateSpeed);
        pitchAngle -= rotateSpeed;
    }
    if (key == 'q') {  // Yaw ���� (Up �� ����)
        rotateAroundAxis(directionX, directionY, directionZ, upX, upY, upZ, rotateSpeed);
        yawAngle += rotateSpeed;
    }
    if (key == 'e') {  // Yaw ������ (Up �� ����)
        rotateAroundAxis(directionX, directionY, directionZ, upX, upY, upZ, -rotateSpeed);
        yawAngle -= rotateSpeed;
    }
    if (key == 'd') {  // Roll ���� (Direction �� ����)
        rotateAroundAxis(upX, upY, upZ, directionX, directionY, directionZ, rotateSpeed);
        rollAngle += rotateSpeed;
    }
    if (key == 'a') {  // Roll ������ (Direction �� ����)
        rotateAroundAxis(upX, upY, upZ, directionX, directionY, directionZ, -rotateSpeed);
        rollAngle -= rotateSpeed;
    }
    if (key == ' ') {
        if (!missile.active) { // �̻����� ��Ȱ��ȭ ������ ���� ����

            missile.active = true;
            missile.Xeye = cameraX;
            missile.Yeye = cameraY;
            missile.Zeye = cameraZ;
            missile.Nx = directionX;
            missile.Ny = directionY;
            missile.Nz = directionZ;

        }
    }

    // ī�޶� Ÿ�� ����
    cameraTargetX = cameraX + directionX;
    cameraTargetY = cameraY + directionY;
    cameraTargetZ = cameraZ + directionZ;
}



float lastTime = 0.0f;

void updatePosition() {
    updateParticles(0.6);

    // ȭ�� ���� ��û
    glutPostRedisplay();
}



void timer(int value) {
    updatePosition();
    glutTimerFunc(16, timer, 0); // �� 60 FPS�� ����
}

void init() {
    //glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    loadBMPTexture("Terrain003.bmp", &terrainTexture);
    loadBMPTexture("water.bmp", &lakeTexture);
    loadSkyboxTextures(); // ��ī�̹ڽ� �ؽ�ó �ε�
    //generateTerrain(); // ���� ����
}

int main(int argc, char** argv) {
    const char* filePath = "P-51 Mustang.obj";
    loadOBJ(filePath, &VaseVertices, &VaseFaces);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("OpenGL Terrain");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);



    if (!loadHeightmap("AnyConv.com__Canyon Height Map (2).raw")) {
        return -1;
    }
    glGenTextures(1, &terrainTexture);
    glBindTexture(GL_TEXTURE_2D, terrainTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    unsigned char dummyData[4] = { 255, 255, 255, 255 }; // ��� �ؽ�ó
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, dummyData);

    init();


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(16, timer, 0);
  

    //glutMotionFunc(motion);
    glutKeyboardFunc(handleKeyboard);
    lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    glutIdleFunc(idle);


    glutMainLoop();
    return 0;
}
