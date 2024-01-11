//=============================================================================================
// Mintaprogram: Zold haromszog. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Kovacs Robert Kristof
// Neptun : R92D9T
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"
//Alabbi osztaly forrasa: hiperbolikus haromszogek mintaprogram
class ImmediateModeRenderer2D : public GPUProgram {
	const char* const vertexSource = R"(
		#version 330
		precision highp float;
		layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0

		void main() { gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1); }	
	)";

	const char* const fragmentSource = R"(
		#version 330
		precision highp float;
		uniform vec3 color;
		out vec4 fragmentColor;	

		void main() { fragmentColor = vec4(color, 1); }
	)";

	unsigned int vao, vbo; // we have just a single vao and vbo for everything :-(

	int Prev(std::vector<vec2> polygon, int i) { return i > 0 ? i - 1 : polygon.size() - 1; }
	int Next(std::vector<vec2> polygon, int i) { return i < polygon.size() - 1 ? i + 1 : 0; }

	bool intersect(vec2 p1, vec2 p2, vec2 q1, vec2 q2) {
		return (dot(cross(p2 - p1, q1 - p1), cross(p2 - p1, q2 - p1)) < 0 &&
			dot(cross(q2 - q1, p1 - q1), cross(q2 - q1, p2 - q1)) < 0);
	}

	bool isEar(const std::vector<vec2>& polygon, int ear) {
		int d1 = Prev(polygon, ear), d2 = Next(polygon, ear);
		vec2 diag1 = polygon[d1], diag2 = polygon[d2];
		for (int e1 = 0; e1 < polygon.size(); e1++) { // test edges for intersection
			int e2 = Next(polygon, e1);
			vec2 edge1 = polygon[e1], edge2 = polygon[e2];
			if (d1 == e1 || d2 == e1 || d1 == e2 || d2 == e2) continue;
			if (intersect(diag1, diag2, edge1, edge2)) return false;
		}
		vec2 center = (diag1 + diag2) / 2.0f; // test middle point for being inside
		vec2 infinity(2.0f, center.y);
		int nIntersect = 0;
		for (int e1 = 0; e1 < polygon.size(); e1++) {
			int e2 = Next(polygon, e1);
			vec2 edge1 = polygon[e1], edge2 = polygon[e2];
			if (intersect(center, infinity, edge1, edge2)) nIntersect++;
		}
		return (nIntersect & 1 == 1);
	}

	void Triangulate(const std::vector<vec2>& polygon, std::vector<vec2>& triangles) {
		if (polygon.size() == 3) {
			triangles.insert(triangles.end(), polygon.begin(), polygon.begin() + 2);
			return;
		}

		std::vector<vec2> newPolygon;
		for (int i = 0; i < polygon.size(); i++) {
			if (isEar(polygon, i)) {
				triangles.push_back(polygon[Prev(polygon, i)]);
				triangles.push_back(polygon[i]);
				triangles.push_back(polygon[Next(polygon, i)]);
				newPolygon.insert(newPolygon.end(), polygon.begin() + i + 1, polygon.end());
				break;
			}
			else newPolygon.push_back(polygon[i]);
		}
		Triangulate(newPolygon, triangles); // recursive call for the rest
	}

	std::vector<vec2> Consolidate(const std::vector<vec2> polygon) {
		const float pixelThreshold = 0.01f;
		vec2 prev = polygon[0];
		std::vector<vec2> consolidatedPolygon = { prev };
		for (auto v : polygon) {
			if (length(v - prev) > pixelThreshold) {
				consolidatedPolygon.push_back(v);
				prev = v;
			}
		}
		if (consolidatedPolygon.size() > 3) {
			if (length(consolidatedPolygon.back() - consolidatedPolygon.front()) < pixelThreshold) consolidatedPolygon.pop_back();
		}
		return consolidatedPolygon;
	}

public:
	ImmediateModeRenderer2D() {
		glViewport(0, 0, windowWidth, windowHeight);
		glLineWidth(2.0f); glPointSize(10.0f);

		create(vertexSource, fragmentSource, "outColor");
		glGenVertexArrays(1, &vao); glBindVertexArray(vao);
		glGenBuffers(1, &vbo); 		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);  // attribute array 0
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
	}

	void DrawGPU(int type, std::vector<vec2> vertices, vec3 color) {
		setUniform(color, "color");
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec2), &vertices[0], GL_DYNAMIC_DRAW);
		glDrawArrays(type, 0, vertices.size());
	}

	void DrawPolygon(std::vector<vec2> vertices, vec3 color) {
		std::vector<vec2> triangles;
		Triangulate(Consolidate(vertices), triangles);
		DrawGPU(GL_TRIANGLES, triangles, color);
	}

	~ImmediateModeRenderer2D() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}
};

ImmediateModeRenderer2D* renderer; // vertex and fragment shaders
const int nTesselatedVertices = 30;

//A ket szukseges alakzat osztalya.
class HyperLine {
	std::vector<vec3> points;
	float ido;
	vec3 v0;
	std::vector<vec3> holder;
public:
	HyperLine(vec3 p, vec3 q) {
		holder.push_back(p);
		holder.push_back(q);
		float d = (-p.x * q.x) + (-p.y * q.y) - (-p.z * q.z);
		ido = acoshf(d);
		v0 = (q - p * coshf(ido)) / sinhf(ido);
		for (float i = -10; i < 10; i += 0.1)
		{
			points.push_back((p * coshf(i) + v0*sinhf(i)));
		}
	}
	
	std::vector<vec2> TessellateToPoincare() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < points.size(); i++)
		{
			vissza.push_back(vec2((points[i].x / (points[i].z + 1)) * 0.5 - 0.5, (points[i].y / (points[i].z + 1)) * 0.5 + 0.5));
		}
		return vissza;
	}
	std::vector<vec2> TessellateToKlein() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < points.size(); i++)
		{
			vissza.push_back(vec2(points[i].x / (points[i].z) * 0.5 + 0.5, points[i].y / (points[i].z) * 0.5 + 0.5));
		}
		return vissza;
	}
	std::vector<vec2> TessellateToBott() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < points.size(); i++)
		{
			vec2 v = vec2(points[i].x * 0.5 + 0.5, points[i].y * 0.5 - 0.5);
			if (v.x >= 0 )
				vissza.push_back(v);
		}
		return vissza;
	}
	std::vector<vec2> TessellateToSide() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < points.size(); i++)
		{
			vec2 v = vec2(points[i].x * 0.5 - 0.5, sqrtf((points[i].x * points[i].x) + (points[i].y * points[i].y) + 1) * 0.5 - 1.49);
				vissza.push_back(v);
		}
		return vissza;
	}
	
	std::vector<vec2> TessellateHolderToPoin() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < holder.size(); i++)
		{
			vissza.push_back(vec2((holder[i].x / (holder[i].z + 1)) * 0.5 - 0.5, (holder[i].y / (holder[i].z + 1)) * 0.5 + 0.5));
		}
		return vissza;
	}
	std::vector<vec2> TessellateHolderToKlein() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < holder.size(); i++)
		{
			vissza.push_back(vec2(holder[i].x / (holder[i].z) * 0.5 + 0.5, holder[i].y / (holder[i].z) * 0.5 + 0.5));
		}
		return vissza;
	}
	std::vector<vec2> TessellateHolderToBott() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < holder.size(); i++)
		{
			vec2 v = vec2(holder[i].x * 0.5 + 0.5, holder[i].y * 0.5 - 0.5);
			if (v.x >= 0 && v.y <= 0)
				vissza.push_back(v);
		}
		return vissza;
	}
	std::vector<vec2> TessellateHolderToSide() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < holder.size(); i++)
		{
			vec2 v = vec2(holder[i].x * 0.5 - 0.5, holder[i].z * 0.5 - 1.49);
			if (v.x <= 0 && v.y <= 0)
				vissza.push_back(v);
		}
		return vissza;
	}
	void DrawBottom() {
		renderer->DrawGPU(GL_LINE_STRIP, TessellateToBott(), vec3(1, 1, 1));
		renderer->DrawGPU(GL_POINTS, TessellateHolderToBott(), vec3(1, 0, 0));
	}
	void DrawSide() {
		renderer->DrawGPU(GL_LINE_STRIP, TessellateToSide(), vec3(1, 1, 1));
		renderer->DrawGPU(GL_POINTS, TessellateHolderToSide(), vec3(1, 0, 0));
	}
	void Draw() {
		renderer->DrawGPU(GL_LINE_STRIP, TessellateToPoincare(), vec3(1, 1, 1));
		renderer->DrawGPU(GL_LINE_STRIP, TessellateToKlein(), vec3(1, 1, 1));
		renderer->DrawGPU(GL_POINTS, TessellateHolderToPoin(), vec3(1, 0, 0));
		renderer->DrawGPU(GL_POINTS, TessellateHolderToKlein(), vec3(1, 0, 0));
	}
};

class HyperCircle {
	std::vector<vec3> holder;
	float radius;
	std::vector<vec3> koriv;
	void doItAll() {
		vec2 p1 = vec2((holder[0].x / (holder[0].z + 1)), (holder[0].y / (holder[0].z + 1)));
		vec2 p2 = vec2((holder[1].x / (holder[1].z + 1)), (holder[1].y / (holder[1].z + 1)));
		vec2 p3 = vec2((holder[2].x / (holder[2].z + 1)), (holder[2].y / (holder[2].z + 1)));
		
		vec2 keszpont;
		float d;
		if (p1.x == p2.x && p2.y == p1.y) {
			vec2 v1 = p2 - p3;
			keszpont = p3 + v1*0.5;
			d = sqrtf((p1.x - keszpont.x) * (p1.x - keszpont.x) + (p1.y - keszpont.y) * (p1.y - keszpont.y));
		}
		else if (p2.x == p3.x && p2.y == p3.y) {
			vec2 v1 = p2 - p1;
			keszpont = p1 + v1 * 0.5;
			d = sqrtf((p1.x - keszpont.x) * (p1.x - keszpont.x) + (p1.y - keszpont.y) * (p1.y - keszpont.y));
		}
		else {
			vec2 v1 = p2 - p1;
			vec2 f1 = p1 + v1 * 0.5;
			vec2 v2 = p3 - p2;
			vec2 f2 = p2 + v2 * 0.5;
			float d1 = v1.x * f1.x + v1.y * f1.y;
			float d2 = v2.x * f2.x + v2.y * f2.y;
			float y = (v2.x * d1 - v1.x * d2) / (v1.y * v2.x - v2.y * v1.x);
			float x = (d1 - v1.y * y) / v1.x;
			keszpont = vec2(x, y);
			d = sqrtf((p1.x - x) * (p1.x - x) + (p1.y - y) * (p1.y - y));
		}
		vec2 rv = keszpont - p1;
		for (int i = 0; i < 50; i++) {
			float phi = i * 2.0f * M_PI / 50;
			vec2 pont2d = vec2(((cosf(phi))*d+keszpont.x), (sinf(phi)*d+keszpont.y));
			float cX = 2.0f * pont2d.x / (1.0f + pont2d.x * pont2d.x + pont2d.y * pont2d.y);
			float cY = 2.0f * pont2d.y / (1.0f + pont2d.x * pont2d.x + pont2d.y * pont2d.y);
			koriv.push_back(vec3(cX / sqrtf(1 - (cX * cX) - (cY * cY)), cY / sqrtf(1 - (cX * cX) - (cY * cY)), 1 / sqrtf(1 - (cX * cX) - (cY * cY))));
		}
		vec3 what = vec3(koriv[0].x, koriv[0].y, koriv[0].z);
		koriv.push_back(what);	
	}
public:
	HyperCircle(vec3 p1, vec3 p2, vec3 p3) {
		holder.push_back(p1);
		holder.push_back(p2);
		holder.push_back(p3);
		doItAll();
	}
	std::vector<vec2> TessellateToPoincare() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < koriv.size(); i++)
		{
			vissza.push_back(vec2((koriv[i].x / (koriv[i].z + 1)) * 0.5 - 0.5, (koriv[i].y / (koriv[i].z + 1)) * 0.5 + 0.5));
		}
		return vissza;
	}
	std::vector<vec2> TessellateToKlein() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < koriv.size(); i++)
		{
			vissza.push_back(vec2(koriv[i].x / (koriv[i].z) * 0.5 + 0.5, koriv[i].y / (koriv[i].z) * 0.5 + 0.5));
		}
		return vissza;
	}
	std::vector<vec2> TessellateToBott() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < koriv.size(); i++)
		{
			vec2 v = vec2(koriv[i].x * 0.5 + 0.5, koriv[i].y * 0.5 - 0.5);
			if (v.x >= 0)
				vissza.push_back(v);
		}
		return vissza;
	}
	std::vector<vec2> TessellateToSide() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < koriv.size()-1; i++)
		{
			vec2 v = vec2(koriv[i].x * 0.5 - 0.5, koriv[i].z * 0.5 - 1.49);
			if (v.y <= 2)
			vissza.push_back(v);
		}
		return vissza;
	}
	std::vector<vec2> TessellateHolderToPoin() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < holder.size(); i++)
		{
			vissza.push_back(vec2((holder[i].x / (holder[i].z + 1)) * 0.5 - 0.5, (holder[i].y / (holder[i].z + 1)) * 0.5 + 0.5));
		}
		return vissza;
	}
	std::vector<vec2> TessellateHolderToKlein() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < holder.size(); i++)
		{
			vissza.push_back(vec2(holder[i].x / (holder[i].z) * 0.5 + 0.5, holder[i].y / (holder[i].z) * 0.5 + 0.5));
		}
		return vissza;
	}
	std::vector<vec2> TessellateHolderToBott() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < holder.size(); i++)
		{
			vec2 v = vec2(holder[i].x * 0.5 + 0.5, holder[i].y * 0.5 - 0.5);
			if (v.x >= 0)
				vissza.push_back(v);
		}
		return vissza;
	}
	std::vector<vec2> TessellateHolderToSide() {
		std::vector<vec2> vissza;
		for (size_t i = 0; i < holder.size(); i++)
		{
			vec2 v = vec2(holder[i].x * 0.5 - 0.5, holder[i].z * 0.5 - 1.49);
				vissza.push_back(v);
		}
		return vissza;
	}
	void DrawSideBody() {
		std::vector<vec2> v = TessellateToSide();
		if (v.size() > 2) renderer->DrawPolygon(v, vec3(0.0f, 0.8f, 0.8f));
	}
	void DrawSideLine() {
		std::vector<vec2> v = TessellateToSide();
		if (v.size() > 2) {
			renderer->DrawGPU(GL_LINE_LOOP, v, vec3(1, 1, 1));
			renderer->DrawGPU(GL_POINTS, TessellateHolderToSide(), vec3(1, 0, 0));
		}
	}
	void DrawBottomBody() {
		std::vector<vec2> v = TessellateToBott();
		if(v.size() > 2) renderer->DrawPolygon(v, vec3(0.0f, 0.8f, 0.8f));
	}
	void DrawBottomLine() {
		std::vector<vec2> v = TessellateToBott();
		if (v.size() > 2) {
			renderer->DrawGPU(GL_LINE_LOOP, v, vec3(1, 1, 1));
			renderer->DrawGPU(GL_POINTS, TessellateHolderToBott(), vec3(1, 0, 0));
		}
	}
	void DrawBody() {
		renderer->DrawPolygon(TessellateToPoincare(), vec3(0.0f, 0.8f, 0.8f)); 
		renderer->DrawPolygon(TessellateToKlein(), vec3(0.0f, 0.8f, 0.8f));
	}
	void DrawLine() {	
		renderer->DrawGPU(GL_LINE_LOOP, TessellateToKlein(), vec3(1, 1, 1));
		renderer->DrawGPU(GL_LINE_LOOP, TessellateToPoincare(), vec3(1, 1, 1));
		renderer->DrawGPU(GL_POINTS, TessellateHolderToPoin(), vec3(1, 0, 0));
		renderer->DrawGPU(GL_POINTS, TessellateHolderToKlein(), vec3(1, 0, 0));
	}
};

// The virtual world
//hatter
std::vector<vec2> poincare, klein, side, bottom, fedo1;
//ez rajta van 
std::vector<vec3> hyperbolicUserPoints;
int hasznalatlan = 0;
//kesz alakzatok
std::vector<HyperLine> pLine;
std::vector<HyperCircle> pCircle;
#pragma region Vetites
//Matek innen
std::vector<vec2> calculate4Points(int n) {
	std::vector<vec2> vissza;
	vissza.push_back(vec2((hyperbolicUserPoints[n].x / (hyperbolicUserPoints[n].z + 1)) * 0.5 - 0.5, (hyperbolicUserPoints[n].y / (hyperbolicUserPoints[n].z + 1)) * 0.5 + 0.5));
	vissza.push_back(vec2(hyperbolicUserPoints[n].x / (hyperbolicUserPoints[n].z) * 0.5 + 0.5, hyperbolicUserPoints[n].y / (hyperbolicUserPoints[n].z) * 0.5 + 0.5));
	vec2 v = vec2(hyperbolicUserPoints[n].x * 0.5 + 0.5, hyperbolicUserPoints[n].y * 0.5 - 0.5);
	if (v.x >= 0 && v.y <= 0) vissza.push_back(v);
	v = vec2(hyperbolicUserPoints[n].x * 0.5 - 0.5, hyperbolicUserPoints[n].z * 0.5 - 1.49);
	if (v.y <= 0 && v.x <= 0)  vissza.push_back(v);
	return vissza;
}
void Kleinfel(float CX, float CY) {
	float cX = CX;
	float cY = CY;
	cX -= 0.5f;
	cX *= 2.0f;
	cY -= 0.5f;
	cY *= 2.0f;
	hyperbolicUserPoints.push_back(vec3(cX / sqrtf(1 - (cX * cX) - (cY * cY)), cY / sqrtf(1 - (cX * cX) - (cY * cY)), 1 / sqrtf(1 - (cX * cX) - (cY * cY))));
}
void Poincarefel(float CX, float CY) {
	float cX1 = CX;
	float cY1 = CY;
	cX1 += 0.5f;
	cX1 *= 2.0f;
	cY1 -= 0.5f;
	cY1 *= 2.0f;
	float cX = 2 * cX1 / (1 + cX1 * cX1 + cY1 * cY1);
	float cY = 2 * cY1 / (1 + cX1 * cX1 + cY1 * cY1);
	hyperbolicUserPoints.push_back(vec3(cX / sqrtf(1 - (cX * cX) - (cY * cY)), cY / sqrtf(1 - (cX * cX) - (cY * cY)), 1 / sqrtf(1 - (cX * cX) - (cY * cY))));
}
void Bottomfel(float CX, float CY) {
	float cX = CX;
	float cY = CY;
	cX -= 0.5f;
	cX *= 2.0f;
	cY += 0.5f;
	cY *= 2.0f;
	hyperbolicUserPoints.push_back(vec3(cX, cY, sqrtf((cX * cX) + (cY * cY) + 1)));
}
void Sidefel(float CX, float CY) {
	float cX = CX;
	float cY = CY;
	cX += 0.5f;
	cX *= 2.0f;
	cY += 1.5f;
	cY *= 2.0f;
	hyperbolicUserPoints.push_back(vec3(cX, -sqrtf(cY * cY - cX * cX - 1), cY));
}
#pragma endregion

// Initialization, create an OpenGL context Done!
void onInitialization() {
	renderer = new ImmediateModeRenderer2D();
	for (int i = 0; i < nTesselatedVertices; i++) {
		float phi = i * 2.0f * M_PI / nTesselatedVertices;
		poincare.push_back(vec2(cosf(phi)*0.5-0.5, sinf(phi)*0.5+0.5));
		klein.push_back(vec2(cosf(phi) * 0.5 + 0.5, sinf(phi) * 0.5 + 0.5));
	}
	for (float i = -1; i < +1.1; i += 0.1) side.push_back(vec2(i-0.5, i * i-1));
	bottom.push_back(vec2(0,0));
	bottom.push_back(vec2(1,0));
	bottom.push_back(vec2(1,-1));
	bottom.push_back(vec2(0,-1));

	fedo1.push_back(vec2(-1, 1));
	fedo1.push_back(vec2(1, 1));
	fedo1.push_back(vec2(1, 0));
	fedo1.push_back(vec2(-1, 0));
}

// Window has become invalid: Redraw
void onDisplay() {
		glClearColor(0.7f, 0.7f, 0.7f, 0);							// background color 
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
		renderer->DrawPolygon(side, vec3(0.5f, 0.5f, 0.5f));
		for (auto p : pCircle) p.DrawSideBody();
		for (auto p : pCircle) p.DrawSideLine();
		for (auto p : pLine) p.DrawSide();

		renderer->DrawGPU(GL_TRIANGLE_FAN, bottom, vec3(0.5f, 0.5f, 0.5f));
		for (auto p : pCircle) p.DrawBottomBody();
		for (auto p : pCircle) p.DrawBottomLine();
		for (auto p : pLine) p.DrawBottom();

		renderer->DrawGPU(GL_TRIANGLE_FAN, fedo1, vec3(0.7f, 0.7f, 0.7f));
		renderer->DrawGPU(GL_TRIANGLE_FAN, poincare, vec3(0.5f, 0.5f, 0.5f));
		renderer->DrawGPU(GL_TRIANGLE_FAN, klein, vec3(0.5f, 0.5f, 0.5f));
		for (auto p : pCircle) p.DrawBody();
		for (auto p : pCircle) p.DrawLine();
		for (auto p : pLine) p.Draw();
		for (size_t i = 0; i < hyperbolicUserPoints.size(); i++)
		{
			renderer->DrawGPU(GL_POINTS, calculate4Points(i), vec3(0, 0, 1));
		}
		glutSwapBuffers();									// exchange the two buffers
}


// Mouse click event
bool pontLerakas(int pX, int pY) {
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
	printf("%f, %f\n", cX, cY);
	if (((cX + 0.5) * (cX + 0.5) + (cY - 0.5) * (cY - 0.5)) < 0.25) {
		Poincarefel(cX, cY);
		return true;
	}
	else if (((cX - 0.5) * (cX - 0.5) + (cY - 0.5) * (cY - 0.5)) < 0.25) {
		Kleinfel(cX, cY);
		return true;
	}
	else if (cX >= 0 && cY <= 0) {
		Bottomfel(cX, cY);
		return true;
	}
	else if (cX <= 0 && cY <= 0) {
		Sidefel(cX, cY);
		return true;
	}
	return false;
}
void onMouse(int button, int state, int pX, int pY) {
	if (state == GLUT_DOWN) {
		if (pontLerakas(pX, pY)) {
			hasznalatlan++;
			if (button == GLUT_RIGHT_BUTTON) {
				if (hasznalatlan == 2) {
					int n = hyperbolicUserPoints.size() - 1;
					pLine.push_back(HyperLine(hyperbolicUserPoints[n], hyperbolicUserPoints[n - 1]));
					hyperbolicUserPoints.pop_back();
					hyperbolicUserPoints.pop_back();
					hasznalatlan -= 2;
				}
				else if (hasznalatlan >= 3) {
					int n = hyperbolicUserPoints.size() - 1;
					pCircle.push_back(HyperCircle(hyperbolicUserPoints[n], hyperbolicUserPoints[n - 1], hyperbolicUserPoints[n - 2]));
					hyperbolicUserPoints.pop_back();
					hyperbolicUserPoints.pop_back();
					hyperbolicUserPoints.pop_back();
					hasznalatlan -= 3;
				}
			}
			glutPostRedisplay();
		}
	}
}
#pragma region NemHasznaltEsemenykezelok
// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {}
// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {}
// Move mouse with key pressed
void onMouseMotion(int pX, int pY) { }
// Idle event indicating that some time elapsed: do animation here
void onIdle() { }
#pragma endregion

