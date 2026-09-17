extern int gsx;
extern int gsy;
//ahhh
extern int gcx;
extern int gcy;
extern const int ssx;
extern const int ssy;
extern float scalefactor;
extern int scx;
extern int scy;


typedef struct {
    int x, y;
    int w;
    int h;
} rect;

typedef struct {
int x;
int y;
} point;

typedef struct {
int x;
int y;
float r;
} circle;

void dli(int x1, int y1, int x2, int y2, int w, lv_color_t c,lv_layer_t* applayer);

void dri(rect r, lv_color_t c,
lv_layer_t* applayer);
//	void dri(int x, int y, int w, int h, lv_color_t c,lv_layer_t* applayer);


void dleti(int x, int y, int h,char ahh, lv_color_t c,lv_layer_t* applayer);

//bool rpcol(int x1, int y1, int x2, int y2, int x, int y);

//bool rrcol(rect a, rect b);

//bool pccol(point p, circle c); 
