/* @(#) $Revision: 66.2 $ */

#ifdef _NAMESPACE_CLEAN
#define cosh _cosh
#define fcosh _fcosh
#define fpow _fpow
#define fsinh _fsinh
#define ftanh _ftanh
#define sinh _sinh
#define tanh _tanh
#endif /* _NAMESPACE_CLEAN */

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _rcosh rcosh
#define rcosh _rcosh
#endif /* _NAMESPACE_CLEAN */

double rcosh(arg) double *arg;
{
	double cosh();
	return cosh(*arg);
}

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _rfcosh rfcosh
#define rfcosh _rfcosh
#endif

float rfcosh(arg) float *arg;
{
        float fcosh();
        return fcosh(*arg);
}

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _rfpow rfpow
#define rfpow _rfpow
#endif /* _NAMESPACE_CLEAN */

float rfpow(x_arg,y_arg) float *x_arg,*y_arg;
{
        float fpow();
        return fpow((float) *x_arg,(float) *y_arg);
}

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _rfsinh rfsinh
#define rfsinh _rfsinh
#endif

float rfsinh(arg) float *arg;
{
        float fsinh();
        return fsinh(*arg);
}

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _rftanh rftanh
#define rftanh _rftanh
#endif /* _NAMESPACE_CLEAN */

float rftanh(arg) float *arg;
{
        float ftanh();
        return ftanh(*arg);
}

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _rsinh rsinh
#define rsinh _rsinh
#endif /* _NAMESPACE_CLEAN */

double rsinh(arg) double *arg;
{
        double sinh();
        return sinh(*arg);
}

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _rtanh rtanh
#define rtanh _rtanh
#endif /* _NAMESPACE_CLEAN */

double rtanh(arg) double *arg;
{
        double tanh();
        return tanh(*arg);
}
