# [GLSL Funktionen](https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/)

## Matrix, Vec erstellen
n ist 1,...,4
`vec[n](float a,...)` erstellt n-Vektor mit Einträgen a,...
`mat[n](float a)` erstellt n-Matrix mit Einträgen a auf der Hauptdiagonalen.
`mat[n](float a,...)` erstellt n-Matrix mit Einträgen a,... in der Matrix von links oben nach _unten_ zuerst nach rechts unten.
`mat[n](vec[n] a,...)` ertstellt n-Matrix mit Einträgen a,... als _Spalten_.
## Vec adressieren ("swizzling")
vec.(x,y,z,w)
vec.(r,g,b,a)
vec.(s,t,p,q)
vec.[k]
## Vektor Operatoren
`/, +, -, =, /=, +=, -=` funktionieren, wie erwartet für alle gleichen Typen Komponentenweise.
`*` ist für Vektoren und Skalare die _komponentenweise_ multiplikation und für Matrix-Matrix Typen eine normale Matrix-Multiplikation. Für Matrix-Vektor-Multiplikation ist dies wie erwartet die bekannte Operation ![](mult.png)

## generische Funktionen genType: float, vec
Kreuzprodukt: `vec3 cross(vec3 a, vec3 b)`
Skalarprodukt: `float dot(genType a, genType b)`
Länge: `float length(genType a)`
Kreuzprodukt: `float distance(genType a, genType b)`
normalisieren: `genType normalize(genType a)`
n, wenn dot(nRef, i) < 0, sonst -n: `genType faceforward(genType n, genType i, genType nRef)`
reflektiere `i` an dem Normalenvektor `n`: `genType reflect(genType i, genType n)`
breche `i` an dem Normalenvektor `n` um das Verhältnis der Brechungsindizies: `genType reflect(genType i, genType n)`
### generische Funktionen genType: float(, vec)
Exponenzieren: `pow(genType basis,genType exponent)`
e^a: `exp(genType a)`
log_e: `log(genType a)`
2^a: `exp2(genType a)`
log_2: `log2(genType a)`
Wurzel: `sqrt(genType a)`
1/Wurzel: `inversesqrt(genType a)`

## Trigonometrie genType: float, vec
https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/[funktion].xhtml
radian -> grad: `genType degrees(genType)`
grad -> radian: `genType radian(genType)`
sin: `genType sin(genType)`
cos: `genType cos(genType)`
tan: `genType tan(genType)`
arc sin -1-1 -> -π/2-π/2: `genType asin(genType)`
arc cos -1-1 -> 0-π: `genType acos(genType)`
arc tan -> 0-π/2: `genType atan(genType)`
arc tan(x/y) -> 0-π/2: `genType atan(genType x, genType y)`


# [GLM Funktionen](http://www.c-jump.com/bcc/common/Talk3/Math/GLM/GLM.html)
GLM versucht so nah wie möglich an GLSL zu kommen.

## Matrix, Vec erstellen
n ist 1,...,4
- `glm::vec[n] v = glm::vec[n](glm::vec[n-1](float b), float a);` erstellt n-Vektor mit Einträgen b,b,...a

- `glm::mat[n](float a)` erstellt n-Matrix mit Einträgen a auf der Hauptdiagonalen.

- ```C++ 
  float from[[n*n]];
  glm::mat[n] matrix = glm::make_mat[n](from);
  ```
  erstellt n-Matrix mit Einträgen a,... in der Matrix von links oben nach _unten_ zuerst nach rechts unten.

## Matrix funktionen
Verschieben: `glm::translate(matrix, vektor)`
Rotieren: `glm::rotate(matrix, winkel in rad, Rotationsachse)`
Skalieren: `glm::scale(matrix, Skalierungsvektor)`

## Besondere Matrizen erstellen
Perspektive: `glm::perspective(fov, aspect, near, far)`
Identität: `glm::mat[n]`
Andere (dokumentation für details): `glm::frustum() glm::ortho() glm::lookAt() glm::project()`

## glm::value_ptr
`glm::value_ptr(glmType)` gibt den Pointer auf die Werte in `glmType` zurück. Damit kann man dann wie gewohnt in `C/C++` mit arbeiten und diesen auch z.B. an `glUniformMatrix4fv` weitergeben.

## Vec adressieren
"swizzling" ist in C++ schwierig zu implementieren.
vec.[k]

## Vektor Operatoren
`/, +, -, =, /=, +=, -=` funktionieren, wie erwartet für alle gleichen Typen Komponentenweise.
`*` ist für Vektoren und Skalare die _komponentenweise_ multiplikation und für Matrix-Matrix Typen eine normale Matrix-Multiplikation. Für Matrix-Vektor-Multiplikation ist dies wie erwartet die bekannte Operation ![](mult.png)

## generische Funktionen genType: float, vec
Kreuzprodukt: `vec3 glm::cross(vec3 a, vec3 b)`
Skalarprodukt: `float glm::dot(genType a, genType b)`
Länge: `float glm::length(genType a)`
Kreuzprodukt: `float glm::distance(genType a, genType b)`
normalisieren: `genType glm::normalize(genType a)`
n, wenn dot(nRef, i) < 0, sonst -n: `genType glm::faceforward(genType n, genType i, genType nRef)`
reflektiere `i` an dem Normalenvektor `n`: `genType glm::reflect(genType i, genType n)`
breche `i` an dem Normalenvektor `n` um das Verhältnis der Brechungsindizies: `genType glm::reflect(genType i, genType n)`
### generische Funktionen genType: float(, vec)
Exponenzieren: `glm::pow(genType basis,genType exponent)`
e^a: `glm::exp(genType a)`
log_e: `glm::log(genType a)`
2^a: `glm::exp2(genType a)`
log_2: `glm::log2(genType a)`
Wurzel: `glm::sqrt(genType a)`
1/Wurzel: `glm::inversesqrt(genType a)`

## Trigonometrie genType: float, vec
https://glm.g-truc.net/0.9.4/api/a00130.html
radian -> grad: `genType glm::degrees(genType)`
grad -> radian: `genType glm::radian(genType)`
sin: `genType glm::sin(genType)`
cos: `genType glm::cos(genType)`
tan: `genType glm::tan(genType)`
arc sin -1-1 -> -π/2-π/2: `genType glm::asin(genType)`
arc cos -1-1 -> 0-π: `genType glm::acos(genType)`
arc tan -> 0-π/2: `genType glm::atan(genType)`
arc tan(x/y) -> 0-π/2: `genType glm::atan(genType x, genType y)`