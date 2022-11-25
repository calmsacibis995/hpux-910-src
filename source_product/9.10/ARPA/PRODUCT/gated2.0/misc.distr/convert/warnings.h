#define WARN_GENTRACE		0x0001
#define WARN_GATEWAY		0x0002
#define WARN_METRICIN		0x0004
#define WARN_ASIN		0x0008
#define WARN_DEFOUT		0x0010
#define WARN_VALIDATE		0x0020
#define WARN_RECONST		0x0040
#define WARN_FIXMETRIC		0x0080
#define WARN_DEFROUTE		0x0100
#define WARN_EGPNETS		0x0200
#define WARN_VALIDAS		0x0400
#define WARN_ANNCEAS		0x0800

#define WARN_MAX		11

unsigned int messages=0;

char *warnings[]={
	"The egp traceflag option is not part of the general traceflag option anymore.\nTurning on the egp flag in the new config file." ,
	"The gateway <metric> option is no longer available.\nUsing supplier mode in the new config file and using a propagate clause.",
	"Metricin is no longer available for egp neighbors.\nRecommend using the preference capability with egp neighbors.", 
	"ASin has been combined with the AS option.\nUsing the value set with AS, if it is used, otherwise using the old ASin value.", 
	"Defaultout is converted to the propagate out option for egp neighbors.\nTo use the metric value for default, a  propagate clause for the default should be added." , 
	"Validate with egp neighbors is no longer supported.\nFunctionality can be achieved by using accept control statements." , 
	"Reconstmetric is no longer supported." , 
	"Fixedmetric is no longer supported." ,
	"Default routes are now handle by static and propagate statements.\nSuggest verifying that the default route conversion is correct or desirable." ,
	"Egpnetsreachable is no longer supported.\nSuggest using propagate control statements to get similar functionality." ,
	"ValidAS is no longer supported." ,
	"Announcing AS to other ASes is handle through propagate statements.\nSuggest using propagate statements to get similar funtionality."
};
