/* The CGI_C library, by Thomas Boutell, version 2.01. CGI_C is intended
	to be a high-quality API to simplify CGI programming tasks. */

/* Make sure this is only included once. */

#ifndef CGI_C
#define CGI_C 1

/* Bring in standard I/O since some of the functions refer to
	types defined by it, such as FILE *. */

#include "fcgi_stdio.h"
//#include <stdio.h>

/* The various CGI environment variables. Instead of using getenv(),
	the programmer should refer to these, which are always
	valid null-terminated strings (they may be empty, but they 
	will never be null). If these variables are used instead
	of calling getenv(), then it will be possible to save
	and restore CGI environments, which is highly convenient
	for debugging. */

extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiServerSoftware;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiServerName;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiGatewayInterface;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiServerProtocol;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiServerPort;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiRequestMethod;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiPathInfo;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiPathTranslated;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiScriptName;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiQueryString;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiRemoteHost;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiRemoteAddr;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiAuthType;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiRemoteUser;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiRemoteIdent;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiContentType;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiAccept;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiUserAgent;
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiReferrer;

/* Cookies as sent to the server. You can also get them
	individually, or as a string array; see the documentation. */
extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiCookie;

extern 
#ifdef __cplusplus
"C" 
#endif
char *cgiSid;

/* A macro providing the same incorrect spelling that is
	found in the HTTP/CGI specifications */
#define cgiReferer cgiReferrer

/* The number of bytes of data received.
	Note that if the submission is a form submission
	the library will read and parse all the information
	directly from cgiIn; the programmer need not do so. */

extern 
#ifdef __cplusplus
"C" 
#endif
int cgiContentLength;

/* Pointer to CGI output. The cgiHeader functions should be used
	first to output the mime headers; the output HTML
	page, GIF image or other web document should then be written
	to cgiOut by the programmer. In the standard CGIC library,
	cgiOut is always equivalent to stdout. */

extern 
#ifdef __cplusplus
"C" 
#endif
FILE *cgiOut;

/* Pointer to CGI input. The programmer does not read from this.
	We have continued to export it for backwards compatibility
	so that cgic 1.x applications link properly. */

extern 
#ifdef __cplusplus
"C" 
#endif
FILE *cgiIn;

/* Possible return codes from the cgiForm family of functions (see below). */

typedef enum {
	cgiFormSuccess,
	cgiFormTruncated,
	cgiFormBadType,
	cgiFormEmpty,
	cgiFormNotFound,
	cgiFormConstrained,
	cgiFormNoSuchChoice,
	cgiFormMemory,
	cgiFormNoFileName,
	cgiFormNoContentType,
	cgiFormNotAFile,
	cgiFormOpenFailed,
	cgiFormIO,
	cgiFormEOF
} cgiFormResultType;

/* These functions are used to retrieve form data. See
	cgic.html for documentation. */

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormString(
	char *name, char *result, int max);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormStringNoNewlines(
	char *name, char *result, int max);


extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormStringSpaceNeeded(
	char *name, int *length);


extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormStringMultiple(
	char *name, char ***ptrToStringArray);

extern 
#ifdef __cplusplus
"C" 
#endif
void cgiStringArrayFree(char **stringArray);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormInteger(
	char *name, int *result, int defaultV);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormIntegerBounded(
	char *name, int *result, int min, int max, int defaultV);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormDouble(
	char *name, double *result, double defaultV);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormDoubleBounded(
	char *name, double *result, double min, double max, double defaultV);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormSelectSingle(
	char *name, char **choicesText, int choicesTotal, 
	int *result, int defaultV);	


extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormSelectMultiple(
	char *name, char **choicesText, int choicesTotal, 
	int *result, int *invalid);

/* Just an alias; users have asked for this */
#define cgiFormSubmitClicked cgiFormCheckboxSingle

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormCheckboxSingle(
	char *name);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormCheckboxMultiple(
	char *name, char **valuesText, int valuesTotal, 
	int *result, int *invalid);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormRadio(
	char *name, char **valuesText, int valuesTotal, 
	int *result, int defaultV);	

/* The paths returned by this function are the original names of files
	as reported by the uploading web browser and shoult NOT be
	blindly assumed to be "safe" names for server-side use! */
extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileName(
	char *name, char *result, int max);

/* The content type of the uploaded file, as reported by the browser.
	It should NOT be assumed that browsers will never falsify
	such information. */
extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileContentType(
	char *name, char *result, int max);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileSize(
	char *name, int *sizeP);

typedef struct cgiFileStruct *cgiFilePtr;

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileOpen(
	char *name, cgiFilePtr *cfpp);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileRead(
	cgiFilePtr cfp, char *buffer, int bufferSize, int *gotP);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormFileClose(
	cgiFilePtr cfp);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiCookieString(
	char *name, char *result, int max);

extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiCookieInteger(
	char *name, int *result, int defaultV);

cgiFormResultType cgiCookies(
	char ***ptrToStringArray);

/* path can be null or empty in which case a path of / (entire site) is set. 
	domain can be a single web site; if it is an entire domain, such as
	'boutell.com', it should begin with a dot: '.boutell.com' */
extern 
#ifdef __cplusplus
"C" 
#endif
void cgiHeaderCookieSetString(char *name, char *value, 
	int secondsToLive, char *path, char *domain);
extern 
#ifdef __cplusplus
"C" 
#endif
void cgiHeaderCookieSetInteger(char *name, int value,
	int secondsToLive, char *path, char *domain);
extern 
#ifdef __cplusplus
"C" 
#endif
void cgiHeaderLocation(char *redirectUrl);
extern 
#ifdef __cplusplus
"C" 
#endif
void cgiHeaderStatus(int status, char *statusMessage);
extern 
#ifdef __cplusplus
"C" 
#endif
void cgiHeaderContentType(char *mimeType);

typedef enum {
	cgiEnvironmentIO,
	cgiEnvironmentMemory,
	cgiEnvironmentSuccess,
	cgiEnvironmentWrongVersion
} cgiEnvironmentResultType;

extern 
#ifdef __cplusplus
"C" 
#endif
cgiEnvironmentResultType cgiWriteEnvironment(char *filename);
extern cgiEnvironmentResultType cgiReadEnvironment(char *filename);

extern 
#ifdef __cplusplus
"C" 
#endif
int cgiMain();
extern 
#ifdef __cplusplus
"C" 
#endif
int cgiMain_init();


extern 
#ifdef __cplusplus
"C" 
#endif
cgiFormResultType cgiFormEntries(
	char ***ptrToStringArray);

/* Output string with the <, &, and > characters HTML-escaped. 
	's' is null-terminated. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiHtmlEscape(char *s);

/* Output data with the <, &, and > characters HTML-escaped. 
	'data' is not null-terminated; 'len' is the number of
	bytes in 'data'. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiHtmlEscapeData(char *data, int len);

/* Output string with the " character HTML-escaped, and no
	other characters escaped. This is useful when outputting
	the contents of a tag attribute such as 'href' or 'src'.
	's' is null-terminated. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiValueEscape(char *s);

/* Output data with the " character HTML-escaped, and no
	other characters escaped. This is useful when outputting
	the contents of a tag attribute such as 'href' or 'src'.
	'data' is not null-terminated; 'len' is the number of
	bytes in 'data'. Returns cgiFormIO in the event
	of error, cgiFormSuccess otherwise. */
cgiFormResultType cgiValueEscapeData(char *data, int len);

#endif /* CGI_C */

