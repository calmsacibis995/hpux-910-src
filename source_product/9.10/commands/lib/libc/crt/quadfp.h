/* this union can be used for quad floating-point or fixed-point numbers */
typedef union {
    unsigned u[4];
    double d[2];
} QUAD;

/* this union can be used for double floating-point or fixed-point numbers */
typedef union {
    unsigned u[2];
    double d;
} DBL;

/* this union can be used for single floating-point or fixed-point numbers */
typedef union {
    unsigned u;
    float f;
} SGL;

extern QUAD U_Qfadd();
extern QUAD U_Qfsub();
extern QUAD U_Qfmpy();
extern QUAD U_Qfdiv();
extern QUAD U_Qfrem();
extern QUAD U_Qfsqrt();
extern QUAD U_Qfrnd();
extern QUAD U_Qfabs();
extern int U_Qfcmp();

extern QUAD U_Qfcnvff_sgl_to_quad();
extern QUAD U_Qfcnvff_dbl_to_quad();
extern float U_Qfcnvff_quad_to_sgl();
extern double U_Qfcnvff_quad_to_dbl();

extern QUAD U_Qfcnvfx_sgl_to_quad();
extern QUAD U_Qfcnvfx_dbl_to_quad();
extern SGL U_Qfcnvfx_quad_to_sgl();
extern DBL U_Qfcnvfx_quad_to_dbl();
extern QUAD U_Qfcnvfx_quad_to_quad();

extern QUAD U_Qfcnvfxt_sgl_to_quad();
extern QUAD U_Qfcnvfxt_dbl_to_quad();
extern SGL U_Qfcnvfxt_quad_to_sgl();
extern DBL U_Qfcnvfxt_quad_to_dbl();
extern QUAD U_Qfcnvfxt_quad_to_quad();

extern QUAD U_Qfcnvxf_sgl_to_quad();
extern QUAD U_Qfcnvxf_dbl_to_quad();
extern SGL U_Qfcnvxf_quad_to_sgl();
extern DBL U_Qfcnvxf_quad_to_dbl();
extern QUAD U_Qfcnvxf_quad_to_quad();

/* convert formats */
#define QUAD_TO_SGL  0x3
#define QUAD_TO_DBL  0x7
#define SGL_TO_QUAD  0xc
#define DBL_TO_QUAD  0xd
#define QUAD_TO_QUAD 0xf
