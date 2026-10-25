#define _CRT_SECURE_NO_WARNINGS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/freeglut.h>
#endif

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <sys/stat.h>

using namespace std;

// =============================================================
// Doll's Secret Tea Party - Clean Professional Magical World
// This version focuses on organized scenes, clear dialogue,
// slow magical movement, simple tea table, and balanced layout.
// =============================================================

// Textures
// These variables store the BMP image textures used for the room and characters.
GLuint roomTex = 0, girlTex = 0, doll1Tex = 0, doll2Tex = 0;

// Story and animation
// The scene variable controls which part of the story is displayed.
// The timing variables are used to create slow, smooth motion.
int scene = 1;
const int MAX_SCENE = 22;

float girlX = -0.65f;
float lilyMoveX = 0.0f;        // Manual movement for Lily using G key
float dollMoveX = 0.0f;
float dollMoveY = 0.0f;
float scaleFactor = 1.0f;
float rotationAngle = 0.0f;

int sceneStartTime = 0;
float globalTime = 0.0f;
float autoMoveY = 0.0f;
float autoAngle = 0.0f;

float d1_initX = -0.10f;
float d2_initX = 0.20f;
float dollsY = -0.62f;

// =============================================================
// File and Texture Helpers
// These functions load BMP files and convert them into OpenGL textures.
// =============================================================

bool existsFile(string p) {
    struct stat buffer;
    return stat(p.c_str(), &buffer) == 0;
}

string folderPath() {
    string p = __FILE__;
    int pos = (int)p.find_last_of("/\\");
    return p.substr(0, pos + 1);
}

GLuint LoadBMP(const char* filename, bool makeTransparent) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        cout << "Cannot open: " << filename << endl;
        return 0;
    }

    unsigned char header[54];
    fread(header, 1, 54, file);

    if (header[0] != 'B' || header[1] != 'M') {
        cout << "Not BMP file: " << filename << endl;
        fclose(file);
        return 0;
    }

    int dataPos = header[10] | (header[11] << 8) | (header[12] << 16) | (header[13] << 24);
    int width = header[18] | (header[19] << 8) | (header[20] << 16) | (header[21] << 24);
    int height = header[22] | (header[23] << 8) | (header[24] << 16) | (header[25] << 24);
    short bpp = header[28] | (header[29] << 8);

    if (bpp != 24 && bpp != 32) {
        cout << "BMP must be 24-bit or 32-bit: " << filename << endl;
        fclose(file);
        return 0;
    }

    int bytes = bpp / 8;
    int rowSize = ((width * bytes + 3) / 4) * 4;

    unsigned char* data = new unsigned char[width * height * 4];
    unsigned char* row = new unsigned char[rowSize];

    fseek(file, dataPos, SEEK_SET);

    for (int y = 0; y < height; y++) {
        fread(row, 1, rowSize, file);

        for (int x = 0; x < width; x++) {
            int src = x * bytes;
            int dst = ((height - 1 - y) * width + x) * 4;

            unsigned char B = row[src];
            unsigned char G = row[src + 1];
            unsigned char R = row[src + 2];

            data[dst] = R;
            data[dst + 1] = G;
            data[dst + 2] = B;

            unsigned char A = 255;
            if (makeTransparent && R > 215 && G > 215 && B > 215)
                A = 0;

            data[dst + 3] = A;
        }
    }

    delete[] row;
    fclose(file);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    delete[] data;

    cout << "Loaded: " << filename << endl;
    return tex;
}

GLuint findTexture(vector<string> names, bool transparent) {
    string folder = folderPath();

    for (string name : names) {
        vector<string> paths = { folder + name, name };

        for (string p : paths) {
            if (existsFile(p))
                return LoadBMP(p.c_str(), transparent);
        }
    }

    cout << "Texture not found." << endl;
    return 0;
}

// =============================================================
// Basic Drawing Functions
// These simple functions are used to build all hand-drawn objects.
// =============================================================

void rectShape(float x1, float y1, float x2, float y2) {
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

void triangleShape(float x1, float y1, float x2, float y2, float x3, float y3) {
    glBegin(GL_TRIANGLES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glEnd();
}

void circleShape(float x, float y, float r) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < 72; i++) {
        float a = i * 2 * 3.14159f / 72;
        glVertex2f(x + r * cos(a), y + r * sin(a));
    }
    glEnd();
}

void ellipseShape(float x, float y, float rx, float ry) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < 72; i++) {
        float a = i * 2 * 3.14159f / 72;
        glVertex2f(x + rx * cos(a), y + ry * sin(a));
    }
    glEnd();
}

void lineShape(float x1, float y1, float x2, float y2, float width = 1.0f) {
    glLineWidth(width);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

void drawText(float x, float y, string s, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : s)
        glutBitmapCharacter(font, c);
}

// =============================================================
// Texture Background and Image Renderer
// =============================================================

void drawBackgroundTexture() {
    if (roomTex != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, roomTex);
        glColor3f(1, 1, 1);

        glBegin(GL_QUADS);
        glTexCoord2f(0, 1); glVertex2f(-1, -1);
        glTexCoord2f(1, 1); glVertex2f(1, -1);
        glTexCoord2f(1, 0); glVertex2f(1, 1);
        glTexCoord2f(0, 0); glVertex2f(-1, 1);
        glEnd();

        glDisable(GL_TEXTURE_2D);
    }
    else {
        glColor3f(0.94f, 0.84f, 0.80f);
        rectShape(-1, -1, 1, 1);
    }
}

void drawImage(float x, float y, float w, float h, GLuint tex, bool flip, float angle = 0.0f) {
    if (tex == 0) return;

    glPushMatrix();

    glTranslatef(x + w / 2, y + h / 2, 0);
    glRotatef(angle, 0, 0, 1);
    glTranslatef(-(w / 2), -(h / 2), 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);

    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);
    if (!flip) {
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(w, 0);
        glTexCoord2f(1, 1); glVertex2f(w, h);
        glTexCoord2f(0, 1); glVertex2f(0, h);
    }
    else {
        glTexCoord2f(0, 1); glVertex2f(0, 0);
        glTexCoord2f(1, 1); glVertex2f(w, 0);
        glTexCoord2f(1, 0); glVertex2f(w, h);
        glTexCoord2f(0, 0); glVertex2f(0, h);
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glPopMatrix();
}

// =============================================================
// Text and Dialogue
// The narrator box explains the story, while bubbles show character speech.
// =============================================================

void narrator(string line1, string line2 = "") {
    glColor3f(0.98f, 0.92f, 0.84f);
    rectShape(-0.98f, -0.98f, 0.98f, -0.75f);

    glColor3f(0.22f, 0.36f, 0.18f);
    glLineWidth(3);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.98f, -0.98f);
    glVertex2f(0.98f, -0.98f);
    glVertex2f(0.98f, -0.75f);
    glVertex2f(-0.98f, -0.75f);
    glEnd();

    glColor3f(0.10f, 0.07f, 0.03f);
    drawText(-0.92f, -0.84f, line1, GLUT_BITMAP_HELVETICA_18);

    if (line2 != "")
        drawText(-0.92f, -0.92f, line2, GLUT_BITMAP_HELVETICA_18);
}

void bubble(float x, float y, string s) {
    float width = s.length() * 0.024f;
    if (width < 0.18f) width = 0.18f;
    if (width > 0.78f) width = 0.78f;

    glColor3f(1, 1, 1);
    rectShape(x, y, x + width, y + 0.12f);

    glColor3f(0, 0, 0);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + 0.12f);
    glVertex2f(x, y + 0.12f);
    glEnd();

    glColor3f(0, 0, 0);
    drawText(x + 0.02f, y + 0.05f, s, GLUT_BITMAP_HELVETICA_12);
}

// =============================================================
// Original-Style Lily Drawing
// =============================================================

void drawPixelLily(float x, float y, float s, float angle) {
    glPushMatrix();

    glTranslatef(x, y, 0);
    glRotatef(angle, 0, 0, 1);
    glScalef(s, s, 1);

    float px = 0.018f;

    glColor3f(0.20f, 0.18f, 0.18f);
    ellipseShape(0.0f, -0.36f, 0.15f, 0.025f);

    glColor3f(0.02f, 0.02f, 0.025f);
    rectShape(-6.5f * px, -11.0f * px, 6.5f * px, 16.0f * px);
    rectShape(-7.5f * px, -8.0f * px, -5.0f * px, 12.0f * px);
    rectShape(5.0f * px, -8.0f * px, 7.5f * px, 12.0f * px);

    glColor3f(0.035f, 0.035f, 0.040f);
    rectShape(-5.5f * px, 13.0f * px, 5.5f * px, 17.0f * px);
    rectShape(-6.5f * px, 10.0f * px, -3.0f * px, 14.0f * px);
    rectShape(3.0f * px, 10.0f * px, 6.5f * px, 14.0f * px);

    glColor3f(1.0f, 0.80f, 0.68f);
    rectShape(-4.2f * px, 3.5f * px, 4.2f * px, 12.0f * px);
    rectShape(-3.2f * px, 2.0f * px, 3.2f * px, 4.0f * px);

    glColor3f(0.015f, 0.015f, 0.020f);
    rectShape(-5.0f * px, 12.0f * px, 5.0f * px, 15.5f * px);
    rectShape(-4.5f * px, 10.0f * px, -1.0f * px, 13.5f * px);
    rectShape(1.0f * px, 10.0f * px, 4.5f * px, 13.5f * px);
    rectShape(-0.8f * px, 8.7f * px, 0.8f * px, 12.2f * px);

    rectShape(-6.5f * px, -6.0f * px, -4.2f * px, 9.0f * px);
    rectShape(4.2f * px, -6.0f * px, 6.5f * px, 9.0f * px);

    glColor3f(1.0f, 0.80f, 0.68f);
    rectShape(-1.4f * px, 0.5f * px, 1.4f * px, 2.5f * px);

    glColor3f(1.0f, 1.0f, 1.0f);
    rectShape(-4.0f * px, -3.0f * px, 4.0f * px, 1.0f * px);

    glColor3f(1.0f, 0.42f, 0.65f);
    rectShape(-4.2f * px, -10.0f * px, 4.2f * px, -3.0f * px);
    rectShape(-3.2f * px, -14.0f * px, -0.5f * px, -10.0f * px);
    rectShape(0.5f * px, -14.0f * px, 3.2f * px, -10.0f * px);

    glColor3f(0.78f, 0.13f, 0.40f);
    rectShape(-3.5f * px, -3.0f * px, -2.5f * px, 1.0f * px);
    rectShape(2.5f * px, -3.0f * px, 3.5f * px, 1.0f * px);
    rectShape(-4.2f * px, -4.0f * px, 4.2f * px, -3.0f * px);

    glColor3f(1.0f, 0.80f, 0.68f);
    rectShape(-6.0f * px, -8.0f * px, -4.2f * px, -1.0f * px);
    rectShape(4.2f * px, -8.0f * px, 6.0f * px, -1.0f * px);

    rectShape(-3.0f * px, -18.0f * px, -1.0f * px, -14.0f * px);
    rectShape(1.0f * px, -18.0f * px, 3.0f * px, -14.0f * px);

    glColor3f(1, 1, 1);
    rectShape(-3.0f * px, -20.0f * px, -1.0f * px, -18.0f * px);
    rectShape(1.0f * px, -20.0f * px, 3.0f * px, -18.0f * px);

    glColor3f(0.04f, 0.05f, 0.12f);
    rectShape(-4.0f * px, -22.0f * px, -1.0f * px, -20.0f * px);
    rectShape(1.0f * px, -22.0f * px, 4.0f * px, -20.0f * px);

    glColor3f(0.02f, 0.02f, 0.03f);
    rectShape(-3.3f * px, 7.0f * px, -1.6f * px, 9.6f * px);
    rectShape(1.6f * px, 7.0f * px, 3.3f * px, 9.6f * px);

    glColor3f(1, 1, 1);
    rectShape(-2.8f * px, 8.5f * px, -2.3f * px, 9.2f * px);
    rectShape(2.3f * px, 8.5f * px, 2.8f * px, 9.2f * px);

    glColor3f(1.0f, 0.55f, 0.62f);
    rectShape(-4.0f * px, 5.0f * px, -3.0f * px, 6.0f * px);
    rectShape(3.0f * px, 5.0f * px, 4.0f * px, 6.0f * px);

    glColor3f(0.30f, 0.08f, 0.10f);
    rectShape(-0.8f * px, 4.2f * px, 0.8f * px, 4.8f * px);

    glColor3f(1.0f, 0.35f, 0.60f);
    rectShape(-6.0f * px, 11.6f * px, -5.0f * px, 12.7f * px);
    rectShape(-4.7f * px, 11.2f * px, -3.4f * px, 13.0f * px);
    rectShape(-5.0f * px, 10.8f * px, -4.4f * px, 13.3f * px);

    glPopMatrix();
}

// =============================================================
// Mom Character: Similar style to Lily, but taller and darker dress
// =============================================================

void drawMom(float x, float y, float s, float angle) {
    glPushMatrix();

    glTranslatef(x, y, 0);
    glRotatef(angle, 0, 0, 1);
    glScalef(s, s, 1);

    float px = 0.019f;

    // Hair similar to Lily, longer sides
    glColor3f(0.06f, 0.025f, 0.02f);
    rectShape(-6.8f * px, -10.0f * px, 6.8f * px, 16.0f * px);
    rectShape(-8.0f * px, -7.0f * px, -5.0f * px, 13.0f * px);
    rectShape(5.0f * px, -7.0f * px, 8.0f * px, 13.0f * px);

    // Face
    glColor3f(1.0f, 0.80f, 0.68f);
    rectShape(-4.2f * px, 3.5f * px, 4.2f * px, 12.0f * px);
    rectShape(-3.2f * px, 2.0f * px, 3.2f * px, 4.0f * px);

    // Bangs
    glColor3f(0.025f, 0.015f, 0.012f);
    rectShape(-5.0f * px, 12.0f * px, 5.0f * px, 15.5f * px);
    rectShape(-4.6f * px, 10.0f * px, -1.0f * px, 13.6f * px);
    rectShape(1.0f * px, 10.0f * px, 4.6f * px, 13.6f * px);

    // Neck
    glColor3f(1.0f, 0.80f, 0.68f);
    rectShape(-1.5f * px, 0.5f * px, 1.5f * px, 2.5f * px);

    // White shirt, matching Lily's simple upper outfit.
    glColor3f(1.0f, 1.0f, 1.0f);
    rectShape(-4.0f * px, -3.0f * px, 4.0f * px, 1.0f * px);

    // Mom's dress follows Lily's dress structure: top skirt block and two lower parts.
    // The color is blue to keep Mom visually different while keeping the same style.
    glColor3f(0.20f, 0.45f, 0.80f);
    rectShape(-4.2f * px, -10.0f * px, 4.2f * px, -3.0f * px);
    rectShape(-3.2f * px, -14.0f * px, -0.5f * px, -10.0f * px);
    rectShape(0.5f * px, -14.0f * px, 3.2f * px, -10.0f * px);

    // Darker belt/details, similar to Lily's dress detail placement.
    glColor3f(0.12f, 0.28f, 0.55f);
    rectShape(-3.5f * px, -3.0f * px, -2.5f * px, 1.0f * px);
    rectShape(2.5f * px, -3.0f * px, 3.5f * px, 1.0f * px);
    rectShape(-4.2f * px, -4.0f * px, 4.2f * px, -3.0f * px);

    // Arms use the same position and style as Lily's arms.
    glColor3f(1.0f, 0.80f, 0.68f);
    rectShape(-6.0f * px, -8.0f * px, -4.2f * px, -1.0f * px);
    rectShape(4.2f * px, -8.0f * px, 6.0f * px, -1.0f * px);

    // Legs, socks, and shoes match Lily's lower body design.
    glColor3f(1.0f, 0.80f, 0.68f);
    rectShape(-3.0f * px, -18.0f * px, -1.0f * px, -14.0f * px);
    rectShape(1.0f * px, -18.0f * px, 3.0f * px, -14.0f * px);

    glColor3f(1.0f, 1.0f, 1.0f);
    rectShape(-3.0f * px, -20.0f * px, -1.0f * px, -18.0f * px);
    rectShape(1.0f * px, -20.0f * px, 3.0f * px, -18.0f * px);

    glColor3f(0.04f, 0.05f, 0.12f);
    rectShape(-4.0f * px, -22.0f * px, -1.0f * px, -20.0f * px);
    rectShape(1.0f * px, -22.0f * px, 4.0f * px, -20.0f * px);

    // Face details
    glColor3f(0.02f, 0.02f, 0.03f);
    rectShape(-3.3f * px, 7.0f * px, -1.6f * px, 9.6f * px);
    rectShape(1.6f * px, 7.0f * px, 3.3f * px, 9.6f * px);

    glColor3f(1, 1, 1);
    rectShape(-2.8f * px, 8.5f * px, -2.3f * px, 9.2f * px);
    rectShape(2.3f * px, 8.5f * px, 2.8f * px, 9.2f * px);

    glColor3f(0.35f, 0.07f, 0.08f);
    rectShape(-0.9f * px, 4.2f * px, 0.9f * px, 4.8f * px);

    glPopMatrix();
}

// =============================================================
// Clean Objects: Table, cups, cookies, toy box, cat, toys
// Each object is drawn with basic shapes to demonstrate 2D graphics skills.
// =============================================================

void drawCup(float x, float y, float r, float angle = 0.0f) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(angle, 0, 0, 1);

    glColor3f(0.88f, 0.88f, 0.88f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < 40; i++) {
        float a = i * 3.14159f / 20;
        glVertex2f(r + 0.012f * cos(a), 0.012f * sin(a));
    }
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    circleShape(0, 0, r);

    glColor3f(0.42f, 0.20f, 0.08f);
    circleShape(0, 0, r * 0.68f);

    glPopMatrix();
}

void drawBiscuit(float x, float y, float angle = 0.0f) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(angle, 0, 0, 1);

    glColor3f(0.82f, 0.61f, 0.35f);
    circleShape(0, 0, 0.018f);

    glColor3f(0.2f, 0.1f, 0.0f);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    glVertex2f(0, 0);
    glVertex2f(0.006f, 0.006f);
    glVertex2f(-0.006f, -0.006f);
    glEnd();

    glPopMatrix();
}

void drawSimpleTeaTable(bool magic = false) {
    float tableX = 0.05f;
    float tableY = -0.70f;

    // Wooden oval table, simple and clean
    glColor3f(0.62f, 0.36f, 0.14f);
    ellipseShape(tableX, tableY, 0.30f, 0.11f);

    glColor3f(0.80f, 0.55f, 0.24f);
    ellipseShape(tableX, tableY + 0.015f, 0.25f, 0.075f);

    glColor3f(0.48f, 0.24f, 0.09f);
    rectShape(tableX - 0.025f, tableY - 0.23f, tableX + 0.025f, tableY - 0.05f);
    ellipseShape(tableX, tableY - 0.23f, 0.11f, 0.025f);

    float lift = magic ? sin(globalTime * 0.65f) * 0.015f : 0.0f;
    float cupAng = magic ? sin(globalTime * 1.1f) * 6.0f : 0.0f;

    // Exactly simple: two cups and cookies, organized positions
    drawCup(tableX - 0.13f, tableY + 0.020f + lift, 0.023f, cupAng);
    drawCup(tableX + 0.13f, tableY + 0.020f - lift, 0.023f, -cupAng);

    drawBiscuit(tableX - 0.025f, tableY + 0.010f, magic ? globalTime * 20.0f : 0.0f);
    drawBiscuit(tableX + 0.025f, tableY + 0.010f, magic ? -globalTime * 18.0f : 0.0f);
}

void drawStar(float x, float y, float s) {
    glColor3f(1.0f, 0.92f, 0.25f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x, y + s);
    glVertex2f(x - s * 0.35f, y + s * 0.18f);
    glVertex2f(x + s * 0.35f, y + s * 0.18f);

    glVertex2f(x, y - s);
    glVertex2f(x - s * 0.35f, y - s * 0.18f);
    glVertex2f(x + s * 0.35f, y - s * 0.18f);

    glVertex2f(x - s, y);
    glVertex2f(x - s * 0.18f, y + s * 0.35f);
    glVertex2f(x - s * 0.18f, y - s * 0.35f);

    glVertex2f(x + s, y);
    glVertex2f(x + s * 0.18f, y + s * 0.35f);
    glVertex2f(x + s * 0.18f, y - s * 0.35f);
    glEnd();
}

void drawSlowSparkles(float centerX, float centerY, float radius, int count) {
    // Slow, controlled sparkles only around magical objects
    for (int i = 0; i < count; i++) {
        float a = globalTime * 0.35f + i * 6.28318f / count;
        float r = radius * (0.70f + 0.20f * sin(globalTime * 0.5f + i));
        float x = centerX + cos(a) * r;
        float y = centerY + sin(a) * r * 0.70f;
        float s = 0.008f + 0.003f * sin(globalTime * 0.6f + i);
        drawStar(x, y, s);
    }
}

void drawMagicToyBox(float x, float y, float s, float openAmount) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(s, s, 1);

    // Base
    glColor3f(0.02f, 0.16f, 0.60f);
    rectShape(-0.16f, -0.10f, 0.16f, 0.08f);

    glColor3f(0.04f, 0.30f, 0.92f);
    rectShape(-0.14f, -0.08f, 0.14f, 0.055f);

    // Gold borders
    glColor3f(0.95f, 0.75f, 0.20f);
    rectShape(-0.17f, -0.105f, 0.17f, -0.075f);
    rectShape(-0.17f, 0.055f, 0.17f, 0.085f);
    rectShape(-0.17f, -0.105f, -0.14f, 0.085f);
    rectShape(0.14f, -0.105f, 0.17f, 0.085f);

    // Lid
    glPushMatrix();
    glTranslatef(-0.16f, 0.09f, 0);
    glRotatef(-openAmount * 65.0f, 0, 0, 1);

    glColor3f(0.02f, 0.20f, 0.75f);
    rectShape(0.0f, 0.0f, 0.32f, 0.055f);

    glColor3f(0.95f, 0.75f, 0.20f);
    rectShape(0.0f, 0.045f, 0.32f, 0.065f);
    rectShape(0.0f, -0.005f, 0.32f, 0.010f);

    glPopMatrix();

    // Stars on box
    drawStar(-0.095f, 0.010f, 0.020f);
    drawStar(0.020f, 0.025f, 0.016f);
    drawStar(0.105f, -0.035f, 0.014f);
    drawStar(-0.015f, -0.045f, 0.012f);

    // Lock
    glColor3f(0.95f, 0.75f, 0.20f);
    rectShape(-0.025f, -0.030f, 0.025f, 0.020f);
    glColor3f(0.18f, 0.12f, 0.03f);
    circleShape(0.0f, -0.005f, 0.006f);

    if (openAmount > 0.30f)
        drawSlowSparkles(0, 0.14f, 0.12f, 5);

    glPopMatrix();
}

void drawCat(float x, float y, float s, float tailAngle, bool surprised) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(s, s, 1);

    glColor3f(0.86f, 0.55f, 0.24f);
    ellipseShape(0, 0, 0.12f, 0.065f);
    circleShape(0.11f, 0.04f, 0.055f);

    triangleShape(0.08f, 0.085f, 0.10f, 0.145f, 0.125f, 0.085f);
    triangleShape(0.13f, 0.085f, 0.155f, 0.145f, 0.17f, 0.085f);

    glColor3f(1.0f, 0.62f, 0.68f);
    triangleShape(0.095f, 0.095f, 0.105f, 0.125f, 0.118f, 0.095f);
    triangleShape(0.14f, 0.095f, 0.153f, 0.125f, 0.162f, 0.095f);

    glPushMatrix();
    glTranslatef(-0.11f, 0.02f, 0);
    glRotatef(tailAngle, 0, 0, 1);
    glColor3f(0.86f, 0.55f, 0.24f);
    rectShape(-0.02f, -0.015f, -0.13f, 0.015f);
    circleShape(-0.14f, 0.0f, 0.018f);
    glPopMatrix();

    glColor3f(0.60f, 0.34f, 0.15f);
    rectShape(-0.065f, -0.075f, -0.035f, -0.12f);
    rectShape(0.025f, -0.075f, 0.055f, -0.12f);

    glColor3f(0, 0, 0);
    if (surprised) {
        circleShape(0.095f, 0.055f, 0.010f);
        circleShape(0.135f, 0.055f, 0.010f);
        glColor3f(0.30f, 0.05f, 0.04f);
        circleShape(0.115f, 0.020f, 0.010f);
    }
    else {
        rectShape(0.088f, 0.052f, 0.104f, 0.062f);
        rectShape(0.128f, 0.052f, 0.144f, 0.062f);
        glColor3f(0.30f, 0.05f, 0.04f);
        rectShape(0.105f, 0.023f, 0.128f, 0.030f);
    }

    glColor3f(0, 0, 0);
    lineShape(0.13f, 0.035f, 0.19f, 0.050f, 1);
    lineShape(0.13f, 0.030f, 0.19f, 0.030f, 1);
    lineShape(0.09f, 0.035f, 0.03f, 0.050f, 1);
    lineShape(0.09f, 0.030f, 0.03f, 0.030f, 1);

    glPopMatrix();
}

void drawTeddyBear(float x, float y, float s, float armAngle) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(s, s, 1);

    glColor3f(0.55f, 0.30f, 0.13f);
    ellipseShape(0, -0.03f, 0.065f, 0.085f);
    circleShape(0, 0.07f, 0.055f);
    circleShape(-0.045f, 0.115f, 0.025f);
    circleShape(0.045f, 0.115f, 0.025f);

    glPushMatrix();
    glTranslatef(-0.06f, 0.0f, 0);
    glRotatef(armAngle, 0, 0, 1);
    ellipseShape(0, 0, 0.025f, 0.065f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.06f, 0.0f, 0);
    glRotatef(-armAngle, 0, 0, 1);
    ellipseShape(0, 0, 0.025f, 0.065f);
    glPopMatrix();

    ellipseShape(-0.035f, -0.11f, 0.030f, 0.025f);
    ellipseShape(0.035f, -0.11f, 0.030f, 0.025f);

    glColor3f(0.86f, 0.62f, 0.38f);
    ellipseShape(0, -0.03f, 0.038f, 0.050f);
    ellipseShape(0, 0.055f, 0.030f, 0.020f);

    glColor3f(0, 0, 0);
    circleShape(-0.018f, 0.080f, 0.006f);
    circleShape(0.018f, 0.080f, 0.006f);
    circleShape(0, 0.060f, 0.006f);

    glColor3f(0.90f, 0.12f, 0.18f);
    triangleShape(-0.025f, 0.015f, 0.025f, 0.015f, 0.0f, -0.010f);

    glPopMatrix();
}

void drawBunny(float x, float y, float s, float jump) {
    glPushMatrix();
    glTranslatef(x, y + jump, 0);
    glScalef(s, s, 1);

    glColor3f(0.96f, 0.96f, 0.99f);
    ellipseShape(0, -0.035f, 0.060f, 0.080f);
    circleShape(0, 0.055f, 0.050f);
    ellipseShape(-0.025f, 0.130f, 0.018f, 0.075f);
    ellipseShape(0.025f, 0.130f, 0.018f, 0.075f);

    glColor3f(1.0f, 0.65f, 0.75f);
    ellipseShape(-0.025f, 0.130f, 0.009f, 0.045f);
    ellipseShape(0.025f, 0.130f, 0.009f, 0.045f);

    glColor3f(0, 0, 0);
    circleShape(-0.015f, 0.065f, 0.006f);
    circleShape(0.015f, 0.065f, 0.006f);

    glColor3f(1.0f, 0.45f, 0.60f);
    circleShape(0, 0.050f, 0.005f);

    glColor3f(0.85f, 0.85f, 0.90f);
    ellipseShape(-0.035f, -0.110f, 0.035f, 0.020f);
    ellipseShape(0.035f, -0.110f, 0.035f, 0.020f);

    glPopMatrix();
}

void drawBall(float x, float y, float r, float angle) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(angle, 0, 0, 1);

    glColor3f(0.95f, 0.15f, 0.20f);
    circleShape(0, 0, r);
    glColor3f(1.0f, 0.85f, 0.10f);
    rectShape(-r, -r * 0.15f, r, r * 0.15f);
    glColor3f(0.12f, 0.35f, 0.85f);
    rectShape(-r * 0.15f, -r, r * 0.15f, r);

    glPopMatrix();
}

void drawToyTrain(float x, float y, float s) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(s, s, 1);

    glColor3f(0.10f, 0.30f, 0.75f);
    rectShape(-0.11f, -0.04f, 0.05f, 0.05f);
    glColor3f(0.95f, 0.10f, 0.13f);
    rectShape(0.05f, -0.02f, 0.12f, 0.05f);

    glColor3f(0.12f, 0.12f, 0.12f);
    rectShape(-0.09f, 0.05f, -0.06f, 0.10f);
    rectShape(0.06f, 0.05f, 0.09f, 0.11f);

    glColor3f(0.05f, 0.05f, 0.05f);
    circleShape(-0.07f, -0.045f, 0.022f);
    circleShape(0.0f, -0.045f, 0.022f);
    circleShape(0.08f, -0.045f, 0.022f);

    glColor3f(1.0f, 0.92f, 0.20f);
    rectShape(-0.035f, 0.00f, 0.025f, 0.035f);

    glPopMatrix();
}

void drawRobot(float x, float y, float s, float armAngle) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(s, s, 1);

    // Clear robot head
    glColor3f(0.62f, 0.82f, 0.90f);
    rectShape(-0.065f, 0.045f, 0.065f, 0.160f);

    // Robot body
    glColor3f(0.45f, 0.65f, 0.78f);
    rectShape(-0.075f, -0.095f, 0.075f, 0.040f);

    // Screen panel
    glColor3f(0.18f, 0.35f, 0.50f);
    rectShape(-0.045f, -0.045f, 0.045f, 0.010f);

    // Eyes
    glColor3f(0.0f, 0.0f, 0.0f);
    circleShape(-0.025f, 0.110f, 0.009f);
    circleShape(0.025f, 0.110f, 0.009f);

    // Mouth
    glColor3f(0.90f, 0.20f, 0.20f);
    rectShape(-0.025f, 0.075f, 0.025f, 0.085f);

    // Antenna
    glColor3f(0.12f, 0.12f, 0.12f);
    lineShape(0.0f, 0.160f, 0.0f, 0.205f, 2);
    glColor3f(1.0f, 0.80f, 0.20f);
    circleShape(0.0f, 0.215f, 0.014f);

    // Arms
    glColor3f(0.45f, 0.65f, 0.78f);

    glPushMatrix();
    glTranslatef(-0.085f, -0.005f, 0);
    glRotatef(armAngle, 0, 0, 1);
    rectShape(-0.015f, -0.065f, 0.015f, 0.055f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.085f, -0.005f, 0);
    glRotatef(-armAngle, 0, 0, 1);
    rectShape(-0.015f, -0.065f, 0.015f, 0.055f);
    glPopMatrix();

    // Legs
    glColor3f(0.35f, 0.52f, 0.65f);
    rectShape(-0.050f, -0.155f, -0.018f, -0.095f);
    rectShape(0.018f, -0.155f, 0.050f, -0.095f);

    // Feet
    glColor3f(0.12f, 0.12f, 0.12f);
    rectShape(-0.065f, -0.175f, -0.010f, -0.155f);
    rectShape(0.010f, -0.175f, 0.065f, -0.155f);

    glPopMatrix();
}

void drawRockingHorse(float x, float y, float s, float angle) {
    // Removed because the rocking horse shape was not clear in the scene.
    // It is intentionally left empty.
}

// =============================================================
// Magical World Background - organized, calm, no clutter
// The magical world is drawn procedurally without using a background texture.
// =============================================================

void drawGradientSky(float topR, float topG, float topB, float botR, float botG, float botB) {
    glBegin(GL_QUADS);
    glColor3f(botR, botG, botB);
    glVertex2f(-1, -1);
    glVertex2f(1, -1);
    glColor3f(topR, topG, topB);
    glVertex2f(1, 1);
    glVertex2f(-1, 1);
    glEnd();
}

void drawCloud(float x, float y, float s) {
    glColor3f(0.95f, 0.96f, 1.0f);
    ellipseShape(x, y, 0.09f * s, 0.035f * s);
    circleShape(x - 0.045f * s, y + 0.010f * s, 0.035f * s);
    circleShape(x, y + 0.025f * s, 0.045f * s);
    circleShape(x + 0.045f * s, y + 0.010f * s, 0.035f * s);
}

void drawCalmStars() {
    // Slow magical stars: more beautiful, but calm and not scattered randomly.
    float slow = globalTime * 0.18f;
    float pulse = 0.0025f * sin(globalTime * 0.45f);

    // Main fixed stars
    drawStar(-0.78f, 0.78f, 0.014f + pulse);
    drawStar(-0.45f, 0.63f, 0.010f);
    drawStar(-0.12f, 0.82f, 0.012f - pulse);
    drawStar(0.25f, 0.68f, 0.011f);
    drawStar(0.62f, 0.80f, 0.013f + pulse);
    drawStar(0.84f, 0.55f, 0.010f);

    // Small slow-moving magical stars
    drawStar(-0.63f + sin(slow) * 0.018f, 0.42f + cos(slow) * 0.010f, 0.010f + pulse);
    drawStar(0.43f + sin(slow + 1.4f) * 0.016f, 0.39f + cos(slow + 1.4f) * 0.010f, 0.010f - pulse);
    drawStar(0.72f + sin(slow + 2.2f) * 0.014f, 0.28f + cos(slow + 2.2f) * 0.009f, 0.009f + pulse);
    drawStar(-0.30f + sin(slow + 3.0f) * 0.014f, 0.50f + cos(slow + 3.0f) * 0.008f, 0.008f);
    drawStar(0.05f + sin(slow + 4.0f) * 0.012f, 0.58f + cos(slow + 4.0f) * 0.008f, 0.008f + pulse);

    // Very tiny decorative stars near the sky, slow only
    drawStar(-0.88f, 0.55f, 0.007f);
    drawStar(-0.02f, 0.72f, 0.007f);
    drawStar(0.52f, 0.50f, 0.007f);
}

void drawMoon(float x, float y, float r) {
    glColor3f(1.0f, 0.94f, 0.60f);
    circleShape(x, y, r);

    glColor3f(0.08f, 0.10f, 0.36f);
    circleShape(x + r * 0.36f, y + r * 0.08f, r * 0.92f);
}

void drawMagicGrass() {
    // Land
    glColor3f(0.16f, 0.62f, 0.26f);
    ellipseShape(0.0f, -0.74f, 1.20f, 0.28f);

    glColor3f(0.20f, 0.72f, 0.30f);
    ellipseShape(0.0f, -0.69f, 1.08f, 0.20f);

    // Small organized flowers
    glColor3f(1.0f, 0.35f, 0.75f);
    circleShape(-0.78f, -0.70f, 0.010f);
    circleShape(-0.72f, -0.64f, 0.009f);
    circleShape(0.74f, -0.69f, 0.010f);
    circleShape(0.58f, -0.62f, 0.009f);

    glColor3f(1.0f, 0.85f, 0.12f);
    circleShape(-0.18f, -0.63f, 0.008f);
    circleShape(0.30f, -0.66f, 0.008f);

    // Gentle blades of grass, only bottom
    glColor3f(0.08f, 0.42f, 0.16f);
    for (int i = 0; i < 28; i++) {
        float x = -0.92f + i * 0.07f;
        float h = 0.025f + 0.010f * ((i % 3) / 2.0f);
        lineShape(x, -0.88f, x + 0.010f, -0.88f + h, 1);
    }
}

void drawMagicPortalRing(float x, float y, float s, float open) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(s, s, 1);

    glColor3f(0.55f, 0.35f, 1.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 90; i++) {
        float a = i * 2 * 3.14159f / 90;
        glVertex2f(cos(a) * 0.30f * open, sin(a) * 0.20f * open);
    }
    glEnd();

    glColor3f(0.25f, 0.78f, 1.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 90; i++) {
        float a = i * 2 * 3.14159f / 90;
        glVertex2f(cos(a) * 0.23f * open, sin(a) * 0.15f * open);
    }
    glEnd();

    drawSlowSparkles(0, 0, 0.28f * open, 6);

    glPopMatrix();
}

void drawTransitionWorld(float stage) {
    // Stage from 0 to 1. It is a separate transition scene.
    drawGradientSky(
        0.08f + stage * 0.12f, 0.06f + stage * 0.14f, 0.25f + stage * 0.30f,
        0.42f + stage * 0.12f, 0.25f + stage * 0.10f, 0.62f + stage * 0.18f
    );

    float cloudMove = stage * 0.28f + sin(globalTime * 0.25f) * 0.015f;
    drawCloud(-0.70f + cloudMove, 0.55f, 1.2f);
    drawCloud(-0.10f + cloudMove * 0.6f, 0.35f, 0.9f);
    drawCloud(0.55f + cloudMove * 0.4f, 0.62f, 1.0f);

    drawMagicPortalRing(0.05f, -0.30f, 1.0f, 0.4f + 0.6f * stage);
    drawMagicToyBox(-0.62f, -0.54f, 0.85f, stage);

    narrator("The toy box opened a soft path of colors.", "Clouds moved slowly as the room changed into another world.");
}

void drawMagicalWorldBase(bool showMoon = true, bool showPortal = false) {
    drawGradientSky(0.05f, 0.08f, 0.32f, 0.20f, 0.28f, 0.70f);

    if (showMoon) drawMoon(-0.78f, 0.75f, 0.09f);
    drawCalmStars();

    drawCloud(-0.55f + sin(globalTime * 0.08f) * 0.018f, 0.52f, 1.0f);
    drawCloud(0.48f + sin(globalTime * 0.07f) * 0.015f, 0.60f, 0.9f);
    drawCloud(0.05f + sin(globalTime * 0.06f) * 0.015f, 0.38f, 0.7f);

    drawMagicGrass();

    if (showPortal)
        drawMagicPortalRing(0.62f, -0.25f, 0.75f, 1.0f);
}

// =============================================================
// Helper animation functions
// =============================================================

float progress01(int durationMs) {
    int elapsed = glutGet(GLUT_ELAPSED_TIME) - sceneStartTime;
    float p = elapsed / (float)durationMs;
    if (p < 0) p = 0;
    if (p > 1) p = 1;
    return p;
}

float smoothStep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

void calculateAutoMotion(bool active) {
    if (active) {
        // Slower and softer movement for a calmer magical feeling.
        autoMoveY = 0.010f * sin(globalTime * 1.05f);
        autoAngle = 2.6f * sin(globalTime * 0.95f);
    }
    else {
        autoMoveY = 0.0f;
        autoAngle = 0.0f;
    }
}

void drawDolls(float x1, float x2, float y, float w, float h, bool animated, float extraRot = 0.0f) {
    float bob = animated ? autoMoveY : 0.0f;
    float ang = animated ? autoAngle : 0.0f;

    // Arrow keys move the dolls in every scene.
    x1 += dollMoveX;
    x2 += dollMoveX;
    y += dollMoveY;

    drawImage(x1 - w / 2, y - 0.05f + bob, w, h, doll1Tex, true, rotationAngle + ang + extraRot);
    drawImage(x2 - w / 2, y - 0.05f - bob * 0.4f, w, h, doll2Tex, true, rotationAngle - ang - extraRot);
}

void drawSceneNumber() {
    glColor3f(0.08f, 0.08f, 0.08f);
    string label = "Scene " + to_string(scene) + " / " + to_string(MAX_SCENE);
    drawText(0.72f, -0.72f, label, GLUT_BITMAP_HELVETICA_12);
}

// =============================================================
// Story Scenes
// Each scene is separated into its own function to keep the code organized.
// =============================================================

void scene1Opening() {
    drawBackgroundTexture();

    glColor3f(0.05f, 0.10f, 0.35f);
    rectShape(-0.68f, -0.18f, 0.68f, 0.34f);

    glColor3f(0.95f, 0.75f, 0.20f);
    glLineWidth(4);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.68f, -0.18f);
    glVertex2f(0.68f, -0.18f);
    glVertex2f(0.68f, 0.34f);
    glVertex2f(-0.68f, 0.34f);
    glEnd();

    glColor3f(1.0f, 0.92f, 0.45f);
    drawText(-0.42f, 0.18f, "Doll's Secret Tea Party", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(1, 1, 1);
    drawText(-0.43f, 0.04f, "A Clean Magical Story", GLUT_BITMAP_HELVETICA_18);
    drawText(-0.48f, -0.08f, "N next | P previous | R reset | arrows move dolls", GLUT_BITMAP_HELVETICA_12);

    drawMagicToyBox(0.0f, -0.50f, 0.90f, 0.0f);

    narrator("Lily's room looked normal, but the blue toy box had a secret.", "Press N to begin.");
}

void scene2TeaParty() {
    drawBackgroundTexture();

    drawSimpleTeaTable(false);
    drawDolls(d1_initX, d2_initX, dollsY, 0.18f * scaleFactor, 0.27f * scaleFactor, false);

    // Draw the magic box first and lower/left, so it appears behind Lily.
    drawMagicToyBox(-0.65f, -0.55f, 0.52f, 0.0f);

    drawPixelLily((-0.65f + lilyMoveX), -0.45f, 0.95f * scaleFactor, rotationAngle);
    drawCat(-0.55f, -0.70f, 0.75f, sin(globalTime * 0.40f) * 6, false);

    narrator("Lily was playing with Rose and Daisy during a simple tea party.", "Only two cups and cookies were placed neatly on the table.");
    bubble(-0.67f, 0.42f, "Would you like tea?");
}

void scene3MomCalls() {
    drawBackgroundTexture();

    float p = smoothStep(progress01(4200));
    float momX = 1.05f + (-0.23f - 1.05f) * p;

    drawSimpleTeaTable(false);
    drawDolls(d1_initX, d2_initX, dollsY, 0.18f, 0.27f, false);

    // Draw the magic box first and lower/left, so it appears behind Lily.
    drawMagicToyBox(-0.65f, -0.55f, 0.52f, 0.0f);

    drawPixelLily((-0.65f + lilyMoveX), -0.45f, 0.95f, 0);
    drawMom(momX, -0.43f, 1.02f, 0);
    drawCat(-0.55f, -0.70f, 0.75f, sin(globalTime * 0.40f) * 6, false);

    narrator("Lily's mom came to call her for dinner.", "Lily did not notice the blue box beginning to shine.");
    bubble(-0.30f, 0.42f, "Lily, dinner is ready.");
}

void scene4LilyLeaves() {
    drawBackgroundTexture();

    float p = smoothStep(progress01(5400));
    float lx = -0.65f + (0.82f + 0.65f) * p;
    float mx = -0.23f + (1.05f + 0.23f) * p;

    drawSimpleTeaTable(false);
    drawDolls(d1_initX, d2_initX, dollsY, 0.18f, 0.27f, false);

    // Draw the magic box first and lower/left, so it appears behind Lily.
    drawMagicToyBox(-0.65f, -0.55f, 0.52f, 0.05f);

    drawPixelLily(lx, -0.45f, 0.95f, sin(globalTime * 2.0f) * 2);
    drawMom(mx, -0.43f, 1.02f, 0);
    drawCat(-0.55f, -0.70f, 0.75f, sin(globalTime * 0.40f) * 6, false);

    narrator("Lily left the room with her mom.", "The dolls stayed still until the room became quiet.");
    bubble(lx - 0.18f, 0.42f, "I'll come back soon.");
}

void scene5DollsPlan() {
    drawBackgroundTexture();

    calculateAutoMotion(true);

    drawSimpleTeaTable(false);
    drawDolls(d1_initX, d2_initX, dollsY, 0.18f, 0.27f, true);
    drawCat(-0.43f, -0.68f, 0.75f, 15 + sin(globalTime * 0.65f) * 10, true);
    drawMagicToyBox(-0.62f, -0.52f, 0.70f, 0.10f);

    narrator("As soon as Lily left, Rose whispered to Daisy.", "They decided to use the toy box carefully.");
    bubble(-0.22f, 0.45f, "Let's visit the magic world.");
    bubble(0.18f, 0.34f, "Only for a little while.");
}

void scene6BoxOpensInRoom() {
    drawBackgroundTexture();

    calculateAutoMotion(true);
    float open = smoothStep(progress01(5400));

    drawSimpleTeaTable(false);
    drawDolls(-0.18f, 0.27f, -0.58f, 0.18f, 0.27f, true);
    drawCat(-0.38f, -0.68f, 0.75f, 35 + sin(globalTime * 0.55f) * 8, true);
    drawMagicToyBox(-0.62f, -0.52f, 0.78f, open);
    drawSlowSparkles(-0.62f, -0.40f, 0.18f, 6);

    narrator("The blue box opened slowly and softly.", "A glowing path appeared, but nothing jumped out yet.");
    bubble(-0.73f, -0.25f, "Meow?");
}

void scene7TransitionColors() {
    float stage = smoothStep(progress01(6500));
    drawTransitionWorld(stage);

    drawDolls(-0.22f + stage * 0.20f, 0.22f - stage * 0.15f, -0.58f + stage * 0.10f,
        0.18f, 0.27f, true);
    drawCat(-0.45f + stage * 0.20f, -0.68f + stage * 0.08f, 0.75f, 20, true);

    bubble(-0.20f, 0.38f, "Hold hands!");
}

void scene8CloudsOpen() {
    drawGradientSky(0.09f, 0.12f, 0.38f, 0.32f, 0.38f, 0.78f);

    float p = smoothStep(progress01(5800));
    drawCloud(-0.10f - p * 0.90f, 0.50f, 1.5f);
    drawCloud(0.15f + p * 0.95f, 0.45f, 1.3f);
    drawCloud(-0.62f + p * 0.12f, 0.70f, 0.9f);
    drawCloud(0.62f - p * 0.12f, 0.68f, 0.9f);

    if (p > 0.35f) {
        drawMoon(-0.78f, 0.74f, 0.09f);
        drawCalmStars();
        drawMagicGrass();
    }

    drawMagicToyBox(-0.62f, -0.52f, 0.85f, 1.0f);
    drawDolls(-0.10f, 0.25f, -0.52f, 0.18f, 0.27f, true);
    drawCat(-0.40f, -0.66f, 0.75f, 10, true);

    narrator("The clouds moved apart slowly.", "A quiet magical meadow appeared in front of them.");
    bubble(0.05f, 0.38f, "It's beautiful...");
}

void scene9ArriveMeadow() {
    drawMagicalWorldBase(true, false);

    drawMagicToyBox(-0.72f, -0.58f, 0.88f, 1.0f);
    drawDolls(-0.15f, 0.32f, -0.54f, 0.18f, 0.27f, true);
    drawCat(0.64f, -0.67f, 0.75f, sin(globalTime * 0.40f) * 6, false);
    drawSimpleTeaTable(false);

    narrator("They found their clean magical meadow, a safe place for a secret tea under the moonlight.");
    bubble(-0.19f, 0.43f, "Welcome to our secret place.");
}

void scene10TeddyAppears() {
    drawMagicalWorldBase(true, false);

    float p = smoothStep(progress01(4800));
    drawMagicToyBox(-0.72f, -0.58f, 0.88f, 1.0f);

    drawDolls(-0.18f, 0.32f, -0.54f, 0.18f, 0.27f, true);
    drawCat(0.64f, -0.67f, 0.75f, 10, false);
    drawSimpleTeaTable(false);

    drawTeddyBear(-0.72f + p * 0.25f, -0.48f - p * 0.16f, 0.76f, sin(globalTime * 1.5f) * 12);

    narrator("The first friend came out of the box: a polite teddy bear. He offered a gentle wave to everyone ", "and took his place by the table, eager to share stories and enjoy the magical tea party under the stars");
    bubble(-0.58f, -0.30f, "Hello, everyone!");
}

void scene11BunnyAppears() {
    drawMagicalWorldBase(true, false);
    drawSimpleTeaTable(false);

    drawMagicToyBox(-0.72f, -0.58f, 0.88f, 1.0f);
    drawTeddyBear(-0.47f, -0.64f, 0.76f, 0);
    drawDolls(-0.18f, 0.32f, -0.54f, 0.18f, 0.27f, true);
    drawCat(0.64f, -0.67f, 0.75f, 10, false);

    float p = smoothStep(progress01(4800));
    drawBunny(-0.72f + p * 1.22f, -0.55f - p * 0.08f, 0.70f, fabs(sin(globalTime * 0.65f)) * 0.025f);

    narrator("Then a small bunny hopped to the right side of the meadow. She arrived with high energy"," twitching her nose in excitement as she greeted her friends.");
    bubble(0.43f, -0.32f, "Can I join?");
}

void scene12RobotAppears() {
    drawMagicalWorldBase(true, false);
    drawSimpleTeaTable(false);

    drawMagicToyBox(-0.72f, -0.58f, 0.88f, 1.0f);
    drawTeddyBear(-0.47f, -0.64f, 0.76f, 0);
    drawBunny(0.50f, -0.63f, 0.70f, 0);
    drawDolls(-0.18f, 0.32f, -0.54f, 0.18f, 0.27f, true);
    drawCat(0.64f, -0.67f, 0.75f, 10, false);

    float p = smoothStep(progress01(4800));
    drawRobot(-0.47f + p * -0.32f, -0.52f - p * 0.13f, 0.90f, sin(globalTime * 0.70f) * 10);

    narrator("A tiny robot came next, its colorful lights flashing in delight at the sight of the tea party", "It joined the circle with a friendly hum.");
    bubble(-0.12f, -0.28f, "Beep! Party ready.");
}



void scene13HorseAppears() {
    drawMagicalWorldBase(true, false);
    drawSimpleTeaTable(false);

    // The extra repeated teddy was removed to avoid duplicate-looking characters.
    // This scene now focuses on the existing toys in a clean arrangement.
    drawMagicToyBox(-0.72f, -0.58f, 0.88f, 1.0f);
    drawTeddyBear(-0.47f, -0.64f, 0.76f, sin(globalTime * 0.55f) * 5);
    drawBunny(0.50f, -0.63f, 0.70f, 0);
    drawRobot(-0.07f, -0.32f, 0.90f, sin(globalTime * 0.65f) * 8);
    drawToyTrain(0.62f, -0.87f, 0.72f);
    drawDolls(-0.18f, 0.32f, -0.54f, 0.18f, 0.27f, true);
    drawCat(0.64f, -0.67f, 0.75f, 10, false);

    narrator("The toys gathered neatly around the magical tea table.", "Each friend had enough space, so the scene looked clear.");
    bubble(0.18f, -0.28f, "This place feels magical.");
}

void scene14OrganizedTeaTime() {
    drawMagicalWorldBase(true, false);
    drawSimpleTeaTable(false);

    drawMagicToyBox(-0.72f, -0.58f, 0.88f, 1.0f);
    drawTeddyBear(-0.47f, -0.64f, 0.76f, sin(globalTime * 0.65f) * 8);
    drawBunny(0.50f, -0.63f, 0.70f, fabs(sin(globalTime * 0.65f)) * 0.020f);
    drawRobot(-0.07f,-0.32f, 0.90f, sin(globalTime * 0.65f) * 8);
    // The duplicate teddy was removed from this position.
    drawToyTrain(0.62f, -0.87f, 0.72f);

    drawDolls(-0.18f, 0.32f, -0.54f, 0.18f, 0.27f, true);
    drawCat(0.68f, -0.66f, 0.75f, sin(globalTime * 0.40f) * 6, false);

    narrator("They began a quiet magical tea time, enjoying the sweet treats they had prepared together.", "Each sip of tea seemed to sparkle with magic.");
    bubble(-0.24f, 0.42f, "Tea for everyone.");
}

void scene15CalmDance() {
    drawMagicalWorldBase(true, false);

    calculateAutoMotion(true);

    drawMagicToyBox(-0.72f, -0.58f, 0.88f, 1.0f);

    drawDolls(-0.18f + sin(globalTime * 0.9f) * 0.035f, 0.32f + cos(globalTime * 0.9f) * 0.035f,
        -0.54f + fabs(sin(globalTime * 0.55f)) * 0.020f, 0.18f, 0.27f, true, sin(globalTime * 0.55f) * 4);

    drawTeddyBear(-0.47f, -0.64f, 0.76f, sin(globalTime * 0.65f) * 10);
    drawBunny(0.50f, -0.63f, 0.70f, fabs(sin(globalTime * 0.70f)) * 0.030f);
    drawRobot(-0.07f, -0.65f, 0.90f, sin(globalTime * 0.70f) * 9);
    drawCat(0.68f, -0.66f, 0.75f, sin(globalTime * 0.55f) * 10, false);

    narrator("The dance was gentle and magical. They moved together like a soft breeze", " filling the secret meadow with laughter and light");
    bubble(0.12f, 0.42f, "Soft dance time.");
}

void scene16LilyFindsPortal() {
    drawBackgroundTexture();

    float p = smoothStep(progress01(5000));
    float lx = 1.05f + (-0.66f - 1.05f) * p;

    drawSimpleTeaTable(false);
    drawMagicToyBox(-0.62f, -0.52f, 0.78f, 0.60f);
    drawMagicPortalRing(0.02f, -0.28f, 0.85f, 1.0f);
    drawPixelLily((lx + lilyMoveX), -0.45f, 0.95f, 0);

    narrator("Lily returned to the tea party and saw the magic box open and glowing", "She stepped closer and heard tiny voices.");
    bubble(lx - 0.15f, 0.42f, "Where did everyone go?");
}

void scene17LilyArrives() {
    drawMagicalWorldBase(true, true);

    float p = smoothStep(progress01(5000));
    float lx = 0.95f + (-0.68f - 0.95f) * p;

    drawMagicToyBox(-0.72f, -0.58f, 0.88f, 1.0f);
    drawDolls(-0.18f, 0.32f, -0.54f, 0.18f, 0.27f, false);
    drawTeddyBear(-0.47f, -0.64f, 0.76f, 0);
    drawBunny(0.50f, -0.63f, 0.70f, 0);
    drawRobot(-0.07f, -0.65f, 0.90f, 0);
    drawCat(0.68f, -0.66f, 0.75f, 0, true);

    drawPixelLily((lx + lilyMoveX), -0.47f, 0.95f, 0);

    narrator("Lily entered the magical meadow and saw the toys standing still.", "The dolls knew the secret had been discovered.");
    bubble(lx - 0.12f, 0.42f, "You can move?");
}

void scene18SecretPromise() {
    drawMagicalWorldBase(true, false);

    drawMagicToyBox(-0.72f, -0.58f, 0.88f, 1.0f);
    drawSimpleTeaTable(false);
    drawDolls(-0.18f, 0.32f, -0.54f, 0.18f, 0.27f, false);
    drawTeddyBear(-0.47f, -0.64f, 0.76f, 0);
    drawBunny(0.50f, -0.63f, 0.70f, 0);
    drawRobot(-0.07f, -0.65f, 0.90f, 0);
    drawCat(0.68f, -0.66f, 0.75f, 0, false);
    drawPixelLily((-0.68f + lilyMoveX), -0.47f, 0.95f, 0);

    narrator("Rose asked Lily to keep the magical world a secret.", "Lily smiled and promised to protect her friends.");
    bubble(-0.26f, 0.42f, "Please don't tell anyone.");
    bubble(-0.73f, 0.28f, "I promise.");
}

void scene19BackToRoom() {
    drawBackgroundTexture();

    drawSimpleTeaTable(false);
    drawDolls(d1_initX, d2_initX, dollsY, 0.18f, 0.27f, false);

    // The box is drawn before Lily so it appears behind her, not on top of her.
    drawMagicToyBox(-0.65f, -0.55f, 0.52f, 0.0f);

    drawPixelLily((-0.65f + lilyMoveX), -0.45f, 0.95f, 0);
    drawCat(-0.53f, -0.70f, 0.75f, sin(globalTime * 0.35f) * 6, false);

    narrator("They returned to Lily's room before anyone noticed.", "The table was neat again, but Lily knew the secret was real.");
    bubble(-0.72f, 0.42f, "Secret friends forever.");
}

void scene20QuietEnding() {
    drawBackgroundTexture();

    drawSimpleTeaTable(false);
    drawDolls(d1_initX, d2_initX, dollsY, 0.18f, 0.27f, false);

    // The box stays behind Lily.
    drawMagicToyBox(-0.65f, -0.55f, 0.52f, 0.0f);

    drawPixelLily((-0.65f + lilyMoveX), -0.45f, 0.95f, 0);
    drawCat(-0.53f, -0.70f, 0.75f, sin(globalTime * 0.35f) * 6, false);

    narrator("Lily looked at the dolls and smiled quietly.", "Nothing moved, but she knew they were her secret friends.");
    bubble(-0.72f, 0.42f, "I will keep your secret.");
}

void scene21FinalHold() {
    drawBackgroundTexture();

    drawSimpleTeaTable(false);
    drawDolls(d1_initX, d2_initX, dollsY, 0.18f, 0.27f, false);

    // The box stays behind Lily for the final scene too.
    drawMagicToyBox(-0.65f, -0.55f, 0.52f, 0.0f);

    drawPixelLily((-0.65f + lilyMoveX), -0.45f, 0.95f, 0);
    drawCat(-0.53f, -0.70f, 0.75f, sin(globalTime * 0.35f) * 6, false);

    narrator("The magical tea party was over for now.", "But every night, Lily's room could become magical again.");
    bubble(-0.72f, 0.42f, "Secret friends forever.");
}

// =============================================================
// Scene Router
// This switch statement chooses which scene function should be drawn.
// =============================================================

void drawScene() {
    globalTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    switch (scene) {
    case 1: scene1Opening(); break;
    case 2: scene2TeaParty(); break;
    case 3: scene3MomCalls(); break;
    case 4: scene4LilyLeaves(); break;
    case 5: scene5DollsPlan(); break;
    case 6: scene6BoxOpensInRoom(); break;
    case 7: scene7TransitionColors(); break;
    case 8: scene8CloudsOpen(); break;
    case 9: scene9ArriveMeadow(); break;
    case 10: scene10TeddyAppears(); break;
    case 11: scene11BunnyAppears(); break;
    case 12: scene12RobotAppears(); break;
   
    case 13: scene13HorseAppears(); break;
    case 14: scene14OrganizedTeaTime(); break;
    case 15: scene15CalmDance(); break;
    case 16: scene16LilyFindsPortal(); break;
    case 17: scene17LilyArrives(); break;
    case 18: scene18SecretPromise(); break;
    case 19: scene19BackToRoom(); break;
    case 20: scene20QuietEnding(); break;
    case 21: scene21FinalHold(); break;
    default: scene1Opening(); break;
    }

    drawSceneNumber();
}

// =============================================================
// Display and Controls
// Keyboard: N/P scenes, G moves Lily, arrows move dolls, R resets.
// =============================================================

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawScene();
    glutSwapBuffers();
}

void timer(int value) {
    glutPostRedisplay();
    glutTimerFunc(30, timer, 0);
}

void resetProject() {
    scene = 1;
    girlX = -0.65f;
    lilyMoveX = 0.0f;
    dollMoveX = 0;
    dollMoveY = 0;
    scaleFactor = 1.0f;
    rotationAngle = 0.0f;
    sceneStartTime = glutGet(GLUT_ELAPSED_TIME);
}

void key(unsigned char k, int x, int y) {
    if (k == 'n' || k == 'N') {
        if (scene < MAX_SCENE) scene++;
        sceneStartTime = glutGet(GLUT_ELAPSED_TIME);
    }

    if (k == 'p' || k == 'P') {
        if (scene > 1) scene--;
        sceneStartTime = glutGet(GLUT_ELAPSED_TIME);
    }

    if (k == 'g' || k == 'G') {
        // Move Lily to the right manually.
        lilyMoveX += 0.08f;
        if (lilyMoveX > 0.70f)
            lilyMoveX = -0.30f;
    }

    if (k == 'r' || k == 'R') {
        resetProject();
    }

    if (k == '+' || k == '=') {
        scaleFactor += 0.1f;
    }

    if (k == '-' || k == '_') {
        if (scaleFactor > 0.2f) scaleFactor -= 0.1f;
    }

    if (k == 'l' || k == 'L') {
        rotationAngle += 5.0f;
    }

    if (k == 'z' || k == 'Z') {
        rotationAngle -= 5.0f;
    }

    if (k == 27) {
        exit(0);
    }

    glutPostRedisplay();
}

void special(int k, int x, int y) {
    if (k == GLUT_KEY_LEFT) dollMoveX -= 0.05f;
    if (k == GLUT_KEY_RIGHT) dollMoveX += 0.05f;
    if (k == GLUT_KEY_UP) dollMoveY += 0.05f;
    if (k == GLUT_KEY_DOWN) dollMoveY -= 0.05f;

    glutPostRedisplay();
}

// =============================================================
// Initialization
// =============================================================

void init() {
    glClearColor(1, 1, 1, 1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    roomTex = findTexture({ "room.bmp" }, false);
    girlTex = findTexture({ "girl.bmp", "character_3.bmp" }, true);
    doll1Tex = findTexture({ "doll1.bmp", "character_2.bmp" }, true);
    doll2Tex = findTexture({ "doll2.bmp", "character_1.bmp" }, true);

    sceneStartTime = glutGet(GLUT_ELAPSED_TIME);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(900, 650);
    glutCreateWindow("Doll's Secret Tea Party - Clean Magical World");

#ifndef __APPLE__
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        cout << "GLEW initialization warning." << endl;
    }
#endif

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutTimerFunc(30, timer, 0);

    glutMainLoop();
    return 0;
}

 