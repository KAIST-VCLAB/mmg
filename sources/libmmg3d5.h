#ifndef _MMG3DLIB_H
#define _MMG3DLIB_H

#include "chrono.h"

/* Return Values */
#define MG_SUCCESS       0 /**< Return value for success */
#define MG_LOWFAILURE    1 /**< Return value if we have problem in the remesh
                            *   process but we can save a conform mesh */
#define MG_STRONGFAILURE 2 /**< Return value if we fail and the mesh is non-conform */

enum optIntCod
  {
    analysis,\
    imprim,\
    mem,\
    debug,\
    angle,\
    iso,\
    noinsert,\
    noswap,\
    nomove,\
    renum,\
  };
enum optDblCod
  {
    dhd,
    hmin,
    hmax,
    hausd,
    hgrad,
    ls,
  };

typedef struct {
  double   c[3]; /**< coordinates of point */
  int      ref; /**< ref of point */
  int      xp; /**< surface point number */
  int      tmp; /**< tmp: numero of points for the saving (we don't count the unused points)*/
  int      flag; /**< flag to know if we have already treated the point */
  char     tag; /**< contains binary flags :
                   if tag=23=16+4+2+1, then the point is MG_REF, MG_GEO,MG_REQ,MG_BDY */
  char    tagdel; /**< tag for delaunay */
} Point;
typedef Point * pPoint;

typedef struct {
  double   n1[3],n2[3]; /**< normals at boundary vertex;
                           n1!=n2 if the vertex belong to a ridge */
  double   t[3]; /** tangeant at vertex */
} xPoint;
typedef xPoint * pxPoint;

typedef struct {
  int      a,b,ref; /**< extremities and ref of edges */
  char     tag; /**< binary flags */
} Edge;
typedef Edge * pEdge;

typedef struct {
  int      v[3],ref; /** vertices and ref of tria */
  int      base;
  int      edg[3]; /**< edg[i] contains the ref of the i^th edge of triangle */
  int      flag;
  char     tag[3]; /**< tag[i] contains the tag associated to th i^th edge of tri */
} Tria;
typedef Tria * pTria;

typedef struct {
  int      v[4],ref; /** vertices and ref of tetra */
  int      base,mark; //CECILE rajout mark pour delaunay
  int      xt;   /**< xt : number of the surfaces xtetra */
  int      flag;
  char     tag;
  double   qual; /**< quality of element */
} Tetra;
typedef Tetra * pTetra;

typedef struct {
  int      ref[4]; /**< ref[i] : ref de la face opp au pt i;*/
  int      edg[6]; /**< edg[i] contains the ref of the i^th edge of tet */
  char     ftag[4]; /**< ftag[i] contains the tag associated to the i^th face of tet */
  char     tag[6]; /**< tag[i] contains the tag associated to the i^th edge of tet */
  char     ori; /**< orientation of tris of the tetra:
                 * i^th bit of ori is set to 0 when the i^th face is bad orientated */
} xTetra;
typedef xTetra * pxTetra;

/** to store geometric edges */
typedef struct {
  int   a,b,ref,nxt;
  char  tag;
} hgeom;
typedef struct {
  int     siz,max,nxt;
  hgeom  *geom;
} HGeom;

typedef struct {
  double        dhd,hmin,hmax,hgrad,hausd,min[3],max[3],delta,ls;
  int           mem;
  int           renum;
  char          imprim,ddebug,badkal,iso,fem;
  unsigned char noinsert, noswap, nomove;
  mytime        ctim[TIMEMAX];
} Info;

typedef struct {
  int       ver,dim,type;
  int       npi,nai,nei,np,na,nt,ne,npmax,namax,ntmax,nemax,xpmax,xtmax;
  int       base; /**< used with flag to know if an entity has been treated */
  int       mark;//CECILE rajout mark pour delaunay
  int       xp,xt; /**< nb of surfaces points/triangles */
  int       npnil,nenil; /**< nb of first unused point/element */
  int      *adja; /**< tab of tetrahedra adjacency : if adjt[4*i+1+j]=4*k+l then
                     the i^th and k^th tets are adjacent and share their
                     faces j and l (resp.) */
  int      *adjt; /**< tab of triangles adjacency : if adjt[3*i+1+j]=3*k+l then
                     the i^th and k^th triangles are adjacent and share their
                     edges j and l (resp.) */
  char     *namein,*nameout;

  pPoint    point;
  pxPoint   xpoint;
  pTetra    tetra;
  pxTetra   xtetra;
  pTria     tria;
  pEdge     edge;
  HGeom     htab;
} Mesh;
typedef Mesh  * pMesh;

typedef struct {
  int       dim,ver,np,npmax,size,type;
  double   *m;
  char     *namein,*nameout;
} Sol;
typedef Sol * pSol;


extern Info info;

int  MMG5_loadMesh(pMesh );
int  MMG5_saveMesh(pMesh );
int  MMG5_loadMet(pSol );
int  MMG5_saveMet(pMesh mesh,pSol met);

/** stock the user options (opt_i and opt_d) in the "info" structure */
void MMG5_stockOption(int opt_i[9],double opt_d[6],pMesh mesh);

/** initialize to default values opt_i and opt_d */
void MMG5_mmg3dinit(int opt_i[9],double opt_d[6]);


/** library */
int  MMG5_mmg3dlib(int opt_i[9],double opt_d[6],pMesh mesh,pSol sol);

#endif
