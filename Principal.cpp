#include <iostream> // Librería con funciones de entrada y salida de datos por consola
#include <cmath> // Operaciones matemáticas

// Librerías de OpenCV
#include <opencv2/core/core.hpp> // Contiene las funciones básicas para representar imágenes (estructuras de matrices y arreglos, etc.)
#include <opencv2/highgui/highgui.hpp> // Contiene las funciones básicas para crear GUI (interfaz gráfica de usuario)
#include <opencv2/imgproc/imgproc.hpp> // Contiene funciones para procesamiento de imágenes
#include <opencv2/imgcodecs/imgcodecs.hpp> // Contiene los enlaces para carga y manipulación de imágenes en diferentes formatos

// Patrones de intercalado candidatos
#define PATRON_COLUMNAS 0
#define PATRON_FILAS 1
#define PATRON_AJEDREZ 2

using namespace std;
using namespace cv;

string nombresPatron[3] = {"Columnas (j mod 2)", "Filas (i mod 2)", "Ajedrez ((i+j) mod 2)"};

// Devuelve a qué imagen oculta (0 o 1) pertenece el píxel (i,j) según el patrón
int clasePixel(int patron, int i, int j){
    if(patron==PATRON_COLUMNAS)
        return j%2;
    if(patron==PATRON_FILAS)
        return i%2;
    return (i+j)%2;
}

// Evidencia numérica: porcentaje de píxeles grises (B==G==R) en cada clase del patrón.
// Con el patrón correcto una clase es 100% gris (la palabra) y la otra 0% (el número).
void evaluarPatron(Mat img, int patron, double porcGris[2]){
    double cont[2] = {0, 0};
    double grises[2] = {0, 0};
    Vec3b pixel;
    int c = 0;

    for(int i=0;i<img.rows;i++){
        for(int j=0;j<img.cols;j++){
            pixel = img.at<Vec3b>(i,j);
            c = clasePixel(patron, i, j);

            cont[c]++;
            if(pixel[0]==pixel[1] && pixel[1]==pixel[2])
                grises[c]++;
        }
    }

    porcGris[0] = 100.0*grises[0]/cont[0];
    porcGris[1] = 100.0*grises[1]/cont[1];
}

// Separa las dos imágenes ocultas recorriendo los píxeles
void separarImagenes(Mat img, int patron, Mat &img0, Mat &img1){
    int filas = img.rows;
    int cols = img.cols;

    if(patron==PATRON_FILAS)
        filas = img.rows/2;
    else
        cols = img.cols/2; // columnas y ajedrez: cada fila aporta cols/2 píxeles a cada imagen

    img0 = Mat::zeros(Size(cols, filas), CV_8UC3);
    img1 = Mat::zeros(Size(cols, filas), CV_8UC3);

    int fi = 0, cj = 0;

    for(int i=0;i<img.rows;i++){
        for(int j=0;j<img.cols;j++){
            if(patron==PATRON_FILAS){
                fi = i/2;
                cj = j;
            }else{
                fi = i;
                cj = j/2;
            }

            if(clasePixel(patron, i, j)==0)
                img0.at<Vec3b>(fi,cj) = img.at<Vec3b>(i,j);
            else
                img1.at<Vec3b>(fi,cj) = img.at<Vec3b>(i,j);
        }
    }
}

Mat procesar(string ruta){
    Mat img = imread(ruta, IMREAD_COLOR);

    if(img.empty()){
        cout << "No se pudo leer " << ruta << endl;
        return Mat();
    }

    cout << "===== " << ruta << " =====" << endl;

    // 1. Descubrir el patrón de intercalado
    double porcGris[2];
    double mejorPuntaje = -1;
    int mejorPatron = 0;

    for(int p=0;p<3;p++){
        evaluarPatron(img, p, porcGris);
        double puntaje = fabs(porcGris[0]-porcGris[1]);

        cout << nombresPatron[p] << " -> %gris clase0 = " << porcGris[0]
             << " | clase1 = " << porcGris[1] << endl;

        if(puntaje>mejorPuntaje){
            mejorPuntaje = puntaje;
            mejorPatron = p;
        }
    }

    evaluarPatron(img, mejorPatron, porcGris);
    int clasePalabra = (porcGris[0]>porcGris[1]) ? 0 : 1; // la palabra es la imagen gris

    cout << "Patron detectado: " << nombresPatron[mejorPatron] << endl;

    // 2. Separar las dos imágenes ocultas
    Mat img0, img1;
    separarImagenes(img, mejorPatron, img0, img1);

    Mat imgPalabra = (clasePalabra==0) ? img0 : img1;
    Mat imgNumero = (clasePalabra==0) ? img1 : img0;

    // 3a. Palabra: gris de muy bajo contraste -> mediana + ecualización del histograma
    Mat gris, palabra;
    cvtColor(imgPalabra, gris, COLOR_BGR2GRAY);
    medianBlur(gris, gris, 5);
    equalizeHist(gris, palabra);

    // 3b. Número: solo está en la Saturación (HSV) con ruido sal y pimienta -> mediana + umbral
    Mat hsv, numero;
    vector<Mat> canales;
    cvtColor(imgNumero, hsv, COLOR_BGR2HSV);
    split(hsv, canales);
    medianBlur(canales[1], canales[1], 5);
    threshold(canales[1], numero, 0, 255, THRESH_BINARY | THRESH_OTSU);

    // Resultado del archivo: palabra arriba, número abajo
    Mat resultado;
    vconcat(palabra, numero, resultado);

    return resultado;
}


int main(int argc, char *argv[]){

    string rutas[3] = {"practica/practica_1.png", "practica/practica_2.png", "practica/practica_3.png"};
    Mat entradas[3], salidas[3];

    int sep = 10;
    int ancho = 0, altoEntrada = 0, altoSalida = 0;

    for(int k=0;k<3;k++){
        entradas[k] = imread(rutas[k], IMREAD_COLOR);
        salidas[k] = procesar(rutas[k]);

        if(entradas[k].empty() || salidas[k].empty())
            return 1;

        cvtColor(salidas[k], salidas[k], COLOR_GRAY2BGR);

        ancho += max(entradas[k].cols, salidas[k].cols) + sep;
        altoEntrada = max(altoEntrada, entradas[k].rows);
        altoSalida = max(altoSalida, salidas[k].rows);
    }

    // Lienzo: una columna por archivo, input arriba y output abajo (separados por franjas grises)
    Mat lienzo = Mat(Size(ancho - sep, altoEntrada + sep + altoSalida), CV_8UC3, Scalar(128,128,128));

    int x = 0;
    for(int k=0;k<3;k++){
        int anchoColumna = max(entradas[k].cols, salidas[k].cols);

        // cada imagen centrada dentro de su columna
        int xe = x + (anchoColumna - entradas[k].cols)/2;
        int ye = (altoEntrada - entradas[k].rows)/2;
        int xs = x + (anchoColumna - salidas[k].cols)/2;

        entradas[k].copyTo(lienzo(Rect(xe, ye, entradas[k].cols, entradas[k].rows)));
        salidas[k].copyTo(lienzo(Rect(xs, altoEntrada + sep, salidas[k].cols, salidas[k].rows)));
        x += anchoColumna + sep;
    }

    // Ventana redimensionable que mantiene la proporción, con un tamaño inicial de 1200 px de ancho
    namedWindow("Lienzo", WINDOW_NORMAL | WINDOW_KEEPRATIO);
    imshow("Lienzo", lienzo);
    resizeWindow("Lienzo", 1200, 1200*lienzo.rows/lienzo.cols);

    waitKey(0);
    destroyAllWindows();

    return 0;
}
