#include <GL/freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION  // 반드시 stbi_load 함수 정의 전에 추가
#include "stb_image.h"


void rotateAroundAxis(float& vx, float& vy, float& vz, float ax, float ay, float az, float angle);
float cameraX = 0.0f, cameraY = 1.0f, cameraZ = 5.0f; // 초기 카메라 위치
float cameraTargetX = 0.0f, cameraTargetY = 1.0f, cameraTargetZ = 0.0f; // 카메라가 보는 방향
float upX = 0.0f, upY = 1.0f, upZ = 0.0f; // 상향 벡터
float directionX = 0.0f, directionY = 0.0f, directionZ = -1.0f; // 카메라 방향

struct Missile {
    bool active = false;
    float Xeye, Yeye, Zeye; 
    float Nx, Ny, Nz;       
    float speed = 1.0f;     
    float length = 2.0f;    
    float radius = 0.2f;   
} missile;


float pitchAngle = 0.0f; // 위아래 회전 각도
float yawAngle = 0.0f;   // 좌우 회전 각도
float rollAngle = 0.0f;  // 좌우 기울기

float cameraDirectionX = 0.0f; // X 방향
float cameraDirectionY = 0.0f; // Y 방향
float cameraDirectionZ = -1.0f; // Z 방향

#define PI 3.141592
#define BMP_Header_Length 54


float terrain[527][527]; // 지형 데이터
GLuint terrainTexture; // 텍스처 ID
GLuint skyboxTextures[6];


float normalPositionX;
float normalPositionY;
float normalPositionZ;



float lakeHeight = 50.0f; // 호수의 높이
GLuint lakeTexture;       // 호수 텍스처



// 지형 크기와 텍스처
int terrainWidth = 527;
int terrainHeight = 527;
// 충돌 반경
float collisionRadius = 0.5f;

bool isGameOver = false;



typedef struct {
    float x, y, z;
} Vertex;

typedef struct {
    int v1, v2, v3; // 삼각형 인덱스
} Face;

Vertex* VaseVertices = NULL;
Face* VaseFaces = NULL;
int vertexCount = 0, faceCount = 0;
int VaseFaceCount = 0;

// OBJ 파일 읽기
void loadOBJ(const char* filename, Vertex** vertices, Face** faces) {
    printf("로드 중인 파일: %s\n", filename);
    FILE* file;
    fopen_s(&file, filename, "r");           ///obj파일 불러오기
    if (!file) {
        printf("파일을 열 수 없습니다: %s\n", filename);
        exit(1);
    }

    // 임시 카운트
    int tempVertexCount = 0, tempFaceCount = 0;
    char line[128];

    // 첫 번째 패스: 카운트 계산
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') tempVertexCount++;                ///v(정점)을 인식하여 몇개인지 저장
        else if (line[0] == 'f' && line[1] == ' ') tempFaceCount++;                ///f(면)을 인식하여 몇개의 면인지 저장
    }

    // 메모리 할당
    *vertices = (Vertex*)malloc(sizeof(Vertex) * tempVertexCount);           //정점 수만큼 동적으로 메모리 할당
    *faces = (Face*)malloc(sizeof(Face) * tempFaceCount * 2); // 사각형을 분할할 때 2배로 늘려야 함

    // 파일 다시 읽기
    fseek(file, 0, SEEK_SET);          ///파일을 다시 읽어
    int vertexIndex = 0, faceIndex = 0;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            // Vertex 읽기
            sscanf_s(line, "v %f %f %f", &(*vertices)[vertexIndex].x, &(*vertices)[vertexIndex].y, &(*vertices)[vertexIndex].z);    ///데이터 상의 v 가 있는곳을 변수에 저장한다.
            
            vertexIndex++;
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            int v1, v2, v3, v4;
            int t1, t2, t3, t4;

            if (sscanf_s(line, "f %d/%d %d/%d %d/%d %d/%d", &v1, &t1, &v2, &t2, &v3, &t3, &v4, &t4) == 8) {    ///면 데이터를 저장
                // 사각형을 두 삼각형으로 분할
                (*faces)[faceIndex].v1 = v1; (*faces)[faceIndex].v2 = v2; (*faces)[faceIndex].v3 = v3;
                faceIndex++;
                (*faces)[faceIndex].v1 = v1; (*faces)[faceIndex].v2 = v3; (*faces)[faceIndex].v3 = v4;
                faceIndex++;
            }
            else if (sscanf_s(line, "f %d/%d %d/%d %d/%d", &v1, &t1, &v2, &t2, &v3, &t3) == 6) {
                // 삼각형 처리
                (*faces)[faceIndex].v1 = v1; (*faces)[faceIndex].v2 = v2; (*faces)[faceIndex].v3 = v3;
                faceIndex++;
            }
        }
    }

    vertexCount = tempVertexCount;
    VaseFaceCount = faceIndex;  // faceIndex는 이제 처리한 전체 면의 수
    fclose(file);

}

void renderVaseModel() {
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < VaseFaceCount; i++) {
        Vertex v1 = VaseVertices[VaseFaces[i].v1 - 1];
        Vertex v2 = VaseVertices[VaseFaces[i].v2 - 1];
        Vertex v3 = VaseVertices[VaseFaces[i].v3 - 1];

        //printf("Rendering Vertex: (%f, %f, %f)\n", v1.x, v1.y, v1.z);  // 각 정점 출력

        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v2.x, v2.y, v2.z);
        glVertex3f(v3.x, v3.y, v3.z);
    }

    glEnd();
}


//////////////////////////////////////////////////////////////////////// 전역 변수

bool checkCollision(float playerX, float playerZ, float playerY) {
    int gridX = (playerX)+284;
    int gridZ = (playerZ)+284;

    // 지형의 유효 범위 확인

    // 현재 위치의 지형 높이 가져오기
    float terrainHeightAtPlayer = terrain[gridX][gridZ] - 80.0f;
    //std::cout << terrainHeightAtPlayer << std::endl;
    // 플레이어가 지형 높이를 초과하면 충돌
    if (playerY <= terrainHeightAtPlayer + collisionRadius) {
        return true;
    }

    return false;
}

// 파티클 구조체 정의
struct Particle {
    float x, y, z;        // 위치
    float vx, vy, vz;     // 속도
    float lifetime;       // 남은 수명
};
std::vector<Particle> particles;

void initParticle(Particle& p, float startX, float startY, float startZ) {
    p.x = startX;
    p.y = startY;
    p.z = startZ;
    p.vx = ((rand() % 100) / 50.0f - 1.0f) * 2.0f; 
    p.vy = ((rand() % 100) / 50.0f) * 5.0f;        
    p.vz = ((rand() % 100) / 50.0f - 1.0f) * 2.0f; 
    p.lifetime = 3.0f;                             // 초기 수명
    
}
Particle p;
// 파티클 생성 함수
void createParticleEffect(float x, float y, float z) {
    for (int i = 0; i < 10; ++i) { // 파티클 10개 생성

        initParticle(p, x, y, z);
        particles.push_back(p);
    }
    std::cout << "파티클 생성 개수: " << particles.size() << std::endl;
}
// 파티클 업데이트 함수
void updateParticles(float deltaTime) {
    for (auto it = particles.begin(); it != particles.end();) {
        it->x += it->vx * deltaTime;
        it->y += it->vy * deltaTime;
        it->z += it->vz * deltaTime;
        it->vy -= 9.8f * deltaTime; 
        it->lifetime -= deltaTime;

        // 수명이 다한 파티클 제거
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

// 구 렌더링 함수
void drawSphere(float x, float y, float z, float radius) {
    glPushMatrix(); 
    glTranslatef(x, y, z); 
    GLUquadric* quad = gluNewQuadric(); 
    gluSphere(quad, radius, 16, 16); 
    gluDeleteQuadric(quad); 
    glPopMatrix(); 
}

// 파티클 렌더링 함수
void renderParticles() {
    
    for (const auto& p : particles) {
        float alpha = p.lifetime; // 수명 기반 투명도 설정
        glColor4f(1.0f, 0.3f, 0.0f, alpha); // 주황색 파티클
        drawSphere(p.x, p.y, p.z, 0.9f); // 반지름 0.1f의 구 그리기
        std::cout << "파티클 생성 위치: " << p.x << p.y << p.z << std::endl;
    }


}

// 미사일 충돌 검사 함수 (지형 기반)
bool checkMissileCollision(float missileX, float missileZ, float missileY) {
    // 지형의 유효 범위 확인
    int gridX = missileX + 284; // 지형의 x 좌표
    int gridZ = missileZ + 284; // 지형의 z 좌표

    // 현재 위치의 지형 높이 가져오기
    float terrainHeightAtMissile = terrain[gridX][gridZ] - 80.0f;

    // 미사일이 지형 높이보다 낮거나 같으면 충돌로 간주
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

    // heightmap의 픽셀 값을 terrain 배열에 할당
    if (width > 527 || height > 527) {
        std::cerr << "Heightmap size exceeds terrain array limits!" << std::endl;
        stbi_image_free(data);
        return false;
    }

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            terrain[i][j] = (float)data[i * width + j]; // 높이를 0.0~1.0으로 정규화
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
    errno_t err = fopen_s(&file, filename, "rb");  ///BMP파일을 읽어 온다
    if (err != 0) {
        printf("파일을 열 수 없습니다: %s\n", filename);     //읽을 수 없을 때 
        exit(1);
    }

    fread(header, 1, BMP_Header_Length, file);      ///BMP 파일의 기본 정보를 54바이트 크기의 헤더에 저장한다.
    width = *(int*)&header[18];                       
    height = *(int*)&header[22];               ///이미지의 폭과 높이 정보를 저장
    dataPos = *(int*)&header[10];                     ///이미지의 시작 위치
    imageSize = width * height * 3;       ///이미지의 크기 계산  (채널 포함)

    data = (unsigned char*)malloc(imageSize);      ///이미지 데이터를 저장할 메모리를 동적으로 할당
    fseek(file, dataPos, SEEK_SET);      ///이미지 데이터의 시작 위치로 파일 포인터 이동
    fread(data, 1, imageSize, file);      ///이미지 데이터를 파일에서 메모리로 일거온다.
    fclose(file);                                     //파일 닫기

    glGenTextures(1, textureID);      ///위에서 받은 텍스처(이미지) 생성하고 ID 를 반환한다
    glBindTexture(GL_TEXTURE_2D, *textureID);                 //생성된 텍스처를 활성화하고 이후 작업을 이 텍스처에 적용하게 한다.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);           ///이미지의 크기에 맞게 텍스처를 2d 이미지로 설정한다.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);          ////텍스처의 확대 축소할 떄의 필터링
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
        printf("파일을 열 수 없습니다: %s\n", filename);
        exit(1);
    }
    if (!file) {
        printf("BMP 파일을 열 수 없습니다: %s\n", filename);
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
    glBindTexture(GL_TEXTURE_2D, terrainTexture);      ///지형 텍스처
    glBegin(GL_QUADS);

    for (int x = 0; x < terrainWidth - 1; ++x) {
        for (int z = 0; z < terrainHeight - 1; ++z) {
            // 텍스처 좌표를 지형 크기에 따라 정규화
            float texCoordX = (float)x / (float)(terrainWidth - 1);         ////텍스처는 지형 크기에 맞춰서 적용
            float texCoordZ = (float)z / (float)(terrainHeight - 1);    

            glTexCoord2f(texCoordX, texCoordZ);               glVertex3f(x, terrain[x][z], z);          ///(x, y)와 (x+1, y+1) 사이의 사가형으로 지형 생성                          
            glTexCoord2f(texCoordX + 1.0f / terrainWidth, texCoordZ); glVertex3f(x + 1, terrain[x + 1][z], z);
            glTexCoord2f(texCoordX + 1.0f / terrainWidth, texCoordZ + 1.0f / terrainHeight); glVertex3f(x + 1, terrain[x + 1][z + 1], z + 1);
            glTexCoord2f(texCoordX, texCoordZ + 1.0f / terrainHeight); glVertex3f(x, terrain[x][z + 1], z + 1);
        }
    }

    glEnd();
}

void drawLake(int terrainWidth, int terrainHeight) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 투명도 블렌딩

    glBindTexture(GL_TEXTURE_2D, lakeTexture);
    glBegin(GL_QUADS);

    for (int x = 0; x < terrainWidth - 1; ++x) {
        for (int z = 0; z < terrainHeight - 1; ++z) {
            if (terrain[x][z] <= lakeHeight) { // 호수 영역
                float texCoordX = (float)x / (float)(terrainWidth - 1);
                float texCoordZ = (float)z / (float)(terrainHeight - 1);

                glColor4f(0.0f, 0.5f, 1.0f, 0.6f); // 반투명한 파란색

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

    glEnable(GL_DEPTH_TEST); // 스카이박스가 뒤에 보이도록

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

    glEnable(GL_DEPTH_TEST); // 다시 depth test 활성화

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
    float objectX = cameraX + directionX * offset;       ///카메라가 바라보는 방향에 오브젝트 배치
    float objectY = cameraY + directionY * offset;
    float objectZ = cameraZ + directionZ * offset;

   
    glTranslatef(objectX, objectY, objectZ);

 
    float viewMatrix[16];
    
    glGetFloatv(GL_MODELVIEW_MATRIX, viewMatrix);   //모델뷰 행렬을 가져온다.

    // 카메라 회전을 반영한 역변환 행렬 적용
    // 회전만 역으로 적용 (카메라의 방향 벡터를 정반대로 회전)
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

 

    // 크기 조정 및 추가 모델 렌더링
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


    // 카메라가 바라보는 위치 계산
    //camera.updateViewMatrix();
    gluLookAt(
        cameraX, cameraY, cameraZ,           // 카메라 위치
        cameraTargetX, cameraTargetY, cameraTargetZ, // 카메라 타겟
        upX, upY, upZ                        // 갱신된 Up 벡터
    );


    // 미사일 렌더링 및 이동
    if (missile.active) {
        glPushMatrix();
        updateMissile();
        glPopMatrix();

        if (!isGameOver && checkMissileCollision(missile.Xeye, missile.Zeye, missile.Yeye)) {
            std::cout << "폭파!" << std::endl;
            renderParticles();
            //missile.active = false;
            //first = 0;
        }
    }


    camerawY = cameraY;
    // 물속인지 여부에 따른 배경 색상 설정
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
        std::cout << "게임 오버!" << std::endl;
        exit(0);
    }



    // 카메라 위치에 맞게 평면을 이동시킴
    renderObjectInFront();




    glColor3f(1.0f, 1.0f, 1.0f);
    // 스카이박스 렌더링
    glPushMatrix();
    glTranslatef(0.0f, -70.0f, 0.0);
    drawSkybox();
    glPopMatrix();

    // 지형과 호수 렌더링
    glPushMatrix();

    //glScalef(2.0f, 2.0f, 2.0f);
    glTranslatef(-284.0f, 0.0f, -284.0f);
    glTranslatef(0.0f, -80.0f, 0.0f);
    drawTerrain(527, 527);
    drawLake(527, 527); // 호수 그리기
    glPopMatrix();

    glutSwapBuffers();
}

float moveSpeedPerFrame = 0.2f; // 프레임당 이동 속도
void moveForwardPerFrame() {
    // 정면 축에 따라 이동
    cameraX += directionX * moveSpeedPerFrame;
    cameraY += directionY * moveSpeedPerFrame;
    cameraZ += directionZ * moveSpeedPerFrame;

    // 타겟도 동일한 방향으로 이동
    cameraTargetX += directionX * moveSpeedPerFrame;
    cameraTargetY += directionY * moveSpeedPerFrame;
    cameraTargetZ += directionZ * moveSpeedPerFrame;
}
void idle() {
    moveForwardPerFrame(); // 프레임마다 카메라 이동
    glutPostRedisplay();   // 화면 갱신
}




void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 1.0f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

void rotateAroundAxis(float& vx, float& vy, float& vz, float ax, float ay, float az, float angle) {   ///축 ax, ay, az가 있으면 이 축을 기준으로 vx, vy, vz가 회전
    float rad = angle * 3.14159265f / 180.0f; // 각도를 라디안으로 변환
    float cosAngle = cos(rad);
    float sinAngle = sin(rad);

    // 축에 대한 벡터 회전 (로드리게스 회전 공식)   이 공식을 이용하여 해당 축을 기준으로 vx, vy, vz
    float dot = vx * ax + vy * ay + vz * az; // 벡터와 축의 내적
    float crossX = ay * vz - az * vy;        // 축과 벡터의 외적
    float crossY = az * vx - ax * vz;
    float crossZ = ax * vy - ay * vx;

    vx = cosAngle * vx + (1 - cosAngle) * dot * ax + sinAngle * crossX;
    vy = cosAngle * vy + (1 - cosAngle) * dot * ay + sinAngle * crossY;
    vz = cosAngle * vz + (1 - cosAngle) * dot * az + sinAngle * crossZ;
}


float moveSpeed = 0.3f;
void handleKeyboard(unsigned char key, int x, int y) {
    float rotateSpeed = 2.0f;

    // Right 벡터 계산 (방향 벡터와 Up 벡터의 외적)
    float rightX = upY * directionZ - upZ * directionY;
    float rightY = upZ * directionX - upX * directionZ;
    float rightZ = upX * directionY - upY * directionX;

    // 벡터 정규화
    float rightLength = sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
    rightX /= rightLength;
    rightY /= rightLength;
    rightZ /= rightLength;

    if (key == 'w') {  // Pitch 위로 (Right 축 기준)
        rotateAroundAxis(directionX, directionY, directionZ, rightX, rightY, rightZ, rotateSpeed);             ///상대 벡터 x, y, z를 생각하고 여기서 는 오른쪽 벡터를 축으로 회전
        rotateAroundAxis(upX, upY, upZ, rightX, rightY, rightZ, rotateSpeed);                                  ///따라서 정면 벡터, 업 벡터는 오른쪽 축을 기준으로 회전
        pitchAngle += rotateSpeed;
    }
    if (key == 's') {  // Pitch 아래로 (Right 축 기준)
        rotateAroundAxis(directionX, directionY, directionZ, rightX, rightY, rightZ, -rotateSpeed);
        rotateAroundAxis(upX, upY, upZ, rightX, rightY, rightZ, -rotateSpeed);
        pitchAngle -= rotateSpeed;
    }
    if (key == 'q') {  // Yaw 왼쪽 (Up 축 기준)
        rotateAroundAxis(directionX, directionY, directionZ, upX, upY, upZ, rotateSpeed);
        yawAngle += rotateSpeed;
    }
    if (key == 'e') {  // Yaw 오른쪽 (Up 축 기준)
        rotateAroundAxis(directionX, directionY, directionZ, upX, upY, upZ, -rotateSpeed);
        yawAngle -= rotateSpeed;
    }
    if (key == 'd') {  // Roll 왼쪽 (Direction 축 기준)
        rotateAroundAxis(upX, upY, upZ, directionX, directionY, directionZ, rotateSpeed);
        rollAngle += rotateSpeed;
    }
    if (key == 'a') {  // Roll 오른쪽 (Direction 축 기준)
        rotateAroundAxis(upX, upY, upZ, directionX, directionY, directionZ, -rotateSpeed);
        rollAngle -= rotateSpeed;
    }
    if (key == ' ') {
        if (!missile.active) { // 미사일이 비활성화 상태일 때만 생성

            missile.active = true;
            missile.Xeye = cameraX;
            missile.Yeye = cameraY;
            missile.Zeye = cameraZ;
            missile.Nx = directionX;
            missile.Ny = directionY;
            missile.Nz = directionZ;

        }
    }

    // 카메라 타겟 갱신
    cameraTargetX = cameraX + directionX;
    cameraTargetY = cameraY + directionY;
    cameraTargetZ = cameraZ + directionZ;
}



float lastTime = 0.0f;

void updatePosition() {
    updateParticles(0.6);

    // 화면 갱신 요청
    glutPostRedisplay();
}



void timer(int value) {
    updatePosition();
    glutTimerFunc(16, timer, 0); // 약 60 FPS로 갱신
}

void init() {
    //glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    loadBMPTexture("Terrain003.bmp", &terrainTexture);
    loadBMPTexture("water.bmp", &lakeTexture);
    loadSkyboxTextures(); // 스카이박스 텍스처 로드
    //generateTerrain(); // 지형 생성
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
    unsigned char dummyData[4] = { 255, 255, 255, 255 }; // 흰색 텍스처
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
