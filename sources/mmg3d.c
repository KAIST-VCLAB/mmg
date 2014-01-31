/** mmg3d: 3d mesh adaptation
 *
 * Written by Cecile Dobrzynski (IMB), Charles Dapogny and Pascal Frey (LJLL)
 * Copyright (c) 2004- IMB/LJLL.
 * All rights reserved.
 */
#include "mmg3d.h"
#include "shared_func.h"

mytime         MMG5_ctim[TIMEMAX];

static void usage(char *prog) {
  fprintf(stdout,"\nUsage: %s [-v [n]] [opts..] filein [fileout]\n",prog);

  fprintf(stdout,"\n** Generic options :\n");
  fprintf(stdout,"-h      Print this message\n");
  fprintf(stdout,"-v [n]  Tune level of verbosity, [-10..10]\n");
  fprintf(stdout,"-m [n]  Set memory size to n Mbytes\n");
  fprintf(stdout,"-d      Turn on debug mode\n");

  fprintf(stdout,"\n**  File specifications\n");
  fprintf(stdout,"-in  file  input triangulation\n");
  fprintf(stdout,"-out file  output triangulation\n");
  fprintf(stdout,"-sol file  load solution file\n");
#ifdef SINGUL
  fprintf(stdout,"-sf  file load file containing singularities\n");
#endif
#ifdef USE_SCOTCH
  fprintf(stdout,"-rn [n]    Turn on or off the renumbering using SCOTCH [1/0] \n");
#endif
#ifdef SINGUL
  fprintf(stdout,"-sing      Preserve internal singularities\n");
#endif


  fprintf(stdout,"\n**  Parameters\n");
  fprintf(stdout,"-ar val    angle detection\n");
  fprintf(stdout,"-nr        no angle detection\n");
  fprintf(stdout,"-hmin val  minimal mesh size\n");
  fprintf(stdout,"-hmax val  maximal mesh size\n");
  fprintf(stdout,"-hausd val control Hausdorff distance\n");
  fprintf(stdout,"-hgrad val control gradation\n");
  fprintf(stdout,"-ls        levelset meshing \n");
  fprintf(stdout,"-noswap    no edge or face flipping\n");
  fprintf(stdout,"-nomove    no point relocation\n");
  fprintf(stdout,"-noinsert  no point insertion/deletion \n");

  exit(EXIT_FAILURE);
}

static int parsar(int argc,char *argv[],pMesh mesh,pSol met,pSingul sing) {
  int     i;
  char   *ptr, namein[128];

  i = 1;
  while ( i < argc ) {
    if ( *argv[i] == '-' ) {
      switch(argv[i][1]) {
      case '?':
        usage(argv[0]);
        break;

      case 'a':
        if ( !strcmp(argv[i],"-ar") && ++i < argc ) {
          mesh->info.dhd = atof(argv[i]);
          mesh->info.dhd = MG_MAX(0.0, MG_MIN(180.0,mesh->info.dhd));
          mesh->info.dhd = cos(mesh->info.dhd*M_PI/180.0);
        }
        break;
      case 'd':  /* debug */
        mesh->info.ddebug = 1;
        break;
      case 'h':
        if ( !strcmp(argv[i],"-hmin") && ++i < argc )
          mesh->info.hmin = atof(argv[i]);
        else if ( !strcmp(argv[i],"-hmax") && ++i < argc )
          mesh->info.hmax = atof(argv[i]);
        else if ( !strcmp(argv[i],"-hausd") && ++i <= argc ) {
          mesh->info.hausd = atof(argv[i]);
	  if ( mesh->info.hausd <=0 ) {
	    fprintf(stdout,"  ## Warning: hausdorff number must be strictly positive.\n");
	    fprintf(stdout,"  Reset to default value.\n");
	    mesh->info.hausd = 0.01;
	  }
        }
        else if ( !strcmp(argv[i],"-hgrad") && ++i <= argc ) {
          mesh->info.hgrad = atof(argv[i]);
          if ( mesh->info.hgrad < 0.0 )
            mesh->info.hgrad = -1.0;
          else
            mesh->info.hgrad = log(mesh->info.hgrad);
        }
        else
          usage(argv[0]);
        break;
      case 'i':
        if ( !strcmp(argv[i],"-in") ) {
          if ( ++i < argc && isascii(argv[i][0]) && argv[i][0]!='-') {
            ADD_MEM(mesh,(strlen(argv[i])+1)*sizeof(char),"input mesh name",
                    printf("  Exit program.\n");
                    exit(EXIT_FAILURE));
            SAFE_CALLOC(mesh->namein,strlen(argv[i])+1,char);
            strcpy(mesh->namein,argv[i]);
            mesh->info.imprim = 5;
          }else{
            fprintf(stderr,"Missing filname for %c%c\n",argv[i-1][1],argv[i-1][2]);
            usage(argv[0]);
          }
        }
        break;
      case 'l':
        if ( !strcmp(argv[i],"-ls") ) {
          mesh->info.iso = 1;
          if ( ++i < argc && isdigit(argv[i][0]) ) {
            mesh->info.ls = atof(argv[i]);
          }
          else if ( i == argc ) {
            fprintf(stderr,"Missing argument option %c%c\n",argv[i-1][1],argv[i-1][2]);
            usage(argv[0]);
          }
          else i--;
        }
        break;
      case 'm':  /* memory */
        if ( ++i < argc && isdigit(argv[i][0]) ) {
	  if ( atoi(argv[i]) <= 0 ) {
	    fprintf(stdout,"  ## Warning: maximal memory authorized must be strictly positive.\n");
	    fprintf(stdout,"  Reset to default value.\n");
	  }
	  else
	    mesh->info.mem = atoi(argv[i]);
	}
        else {
          fprintf(stderr,"Missing argument option %c\n",argv[i-1][1]);
          usage(argv[0]);
        }
        break;
      case 'n':
        if ( !strcmp(argv[i],"-nr") )
          mesh->info.dhd = -1.0;
        else if ( !strcmp(argv[i],"-noswap") )
          mesh->info.noswap = 1;
        else if( !strcmp(argv[i],"-noinsert") )
          mesh->info.noinsert = 1;
        else if( !strcmp(argv[i],"-nomove") )
          mesh->info.nomove = 1;
        break;
      case 'o':
        if ( !strcmp(argv[i],"-out") ) {
          if ( ++i < argc && isascii(argv[i][0])  && argv[i][0]!='-') {
            ADD_MEM(mesh,(strlen(argv[i])+1)*sizeof(char),"output mesh name",
                    printf("  Exit program.\n");
                    exit(EXIT_FAILURE));
            SAFE_CALLOC(mesh->nameout,strlen(argv[i])+1,char);
            strcpy(mesh->nameout,argv[i]);
          }else{
            fprintf(stderr,"Missing filname for %c%c%c\n",
                    argv[i-1][1],argv[i-1][2],argv[i-1][3]);
            usage(argv[0]);
          }
        }
        break;
#ifdef USE_SCOTCH
      case 'r':
        if ( !strcmp(argv[i],"-rn") ) {
          if ( ++i < argc ) {
            if ( isdigit(argv[i][0]) )
              mesh->info.renum = atoi(argv[i]);
            else {
              fprintf(stderr,"Missing argument option %s\n",argv[i-1]);
              usage(argv[0]);
            }
          }
          else {
            fprintf(stderr,"Missing argument option %s\n",argv[i-1]);
            usage(argv[0]);
          }
        }
        break;
#endif
      case 's':
        if ( !strcmp(argv[i],"-sol") ) {
          if ( ++i < argc && isascii(argv[i][0]) && argv[i][0]!='-' ) {
            ADD_MEM(mesh,(strlen(argv[i])+1)*sizeof(char),"input sol name",
                    printf("  Exit program.\n");
                    exit(EXIT_FAILURE));
            SAFE_CALLOC(met->namein,strlen(argv[i])+1,char);
            strcpy(met->namein,argv[i]);
          }
          else {
            fprintf(stderr,"Missing filname for %c%c%c\n",argv[i-1][1],argv[i-1][2],argv[i-1][3]);
            usage(argv[0]);
          }
        }
#ifdef SINGUL
        else if ( !strcmp(argv[i],"-sf") ) {
          if ( ++i < argc && isascii(argv[i][0]) && argv[i][0]!='-' ) {
            ADD_MEM(mesh,(strlen(argv[i])+1)*sizeof(char),
                    "input singularities file name",
                    printf("  Exit program.\n");
                    exit(EXIT_FAILURE));
            SAFE_CALLOC(sing->namein,strlen(argv[i])+1,char);
            strcpy(sing->namein,argv[i]);
            mesh->info.sing = 1;
          }
          else {
            fprintf(stderr,"Missing filname for %c%c%c\n",
                    argv[i-1][1],argv[i-1][2],argv[i-1][3]);
            usage(argv[0]);
          }
        }
        else if ( !strcmp(argv[i],"-sing") )
          mesh->info.sing = 1;
#endif
        break;
      case 'v':
        if ( ++i < argc ) {
          if ( argv[i][0] == '-' || isdigit(argv[i][0]) )
            mesh->info.imprim = atoi(argv[i]);
          else
            i--;
        }
        else {
          fprintf(stderr,"Missing argument option %c\n",argv[i-1][1]);
          usage(argv[0]);
        }
        break;
      default:
        fprintf(stderr,"Unrecognized option %s\n",argv[i]);
        usage(argv[0]);
      }
    }
    else {
      if ( mesh->namein == NULL ) {
        ADD_MEM(mesh,(strlen(argv[i])+1)*sizeof(char),
                "input mesh name",
                printf("  Exit program.\n");
                exit(EXIT_FAILURE));
        SAFE_CALLOC(mesh->namein,strlen(argv[i])+1,char);
        strcpy(mesh->namein,argv[i]);
        if ( mesh->info.imprim == -99 )  mesh->info.imprim = 5;
      }
      else if ( mesh->nameout == NULL ){
        ADD_MEM(mesh,(strlen(argv[i])+1)*sizeof(char),
                "output mesh name",
                printf("  Exit program.\n");
                exit(EXIT_FAILURE));
        SAFE_CALLOC(mesh->nameout,strlen(argv[i])+1,char);
        strcpy(mesh->nameout,argv[i]);
      }
      else {
        fprintf(stdout,"Argument %s ignored\n",argv[i]);
        usage(argv[0]);
      }
    }
    i++;
  }

  /* check file names */
  if ( mesh->info.imprim == -99 ) {
    fprintf(stdout,"\n  -- PRINT (0 10(advised) -10) ?\n");
    fflush(stdin);
    fscanf(stdin,"%d",&i);
    mesh->info.imprim = i;
  }

  if ( mesh->namein == NULL ) {
    fprintf(stdout,"  -- INPUT MESH NAME ?\n");
    fflush(stdin);
    fscanf(stdin,"%s",namein);
    ADD_MEM(mesh,(strlen(namein)+1)*sizeof(char),"input mesh name",
            printf("  Exit program.\n");
            exit(EXIT_FAILURE));
    SAFE_CALLOC(mesh->namein,strlen(namein)+1,char);
    strcpy(mesh->namein,namein);
  }
  if ( mesh->nameout == NULL ) {
    ADD_MEM(mesh,(strlen(mesh->namein)+3)*sizeof(char),"output mesh name",
            printf("  Exit program.\n");
            exit(EXIT_FAILURE));
    SAFE_CALLOC(mesh->nameout,strlen(mesh->namein)+3,char);
    strcpy(mesh->nameout,mesh->namein);
    ptr = strstr(mesh->nameout,".mesh");
    if ( ptr ) {
      *ptr = '\0';
      strcat(mesh->nameout,".o.mesh");
    }
    else
      strcat(mesh->nameout,".o");
    ptr = strstr(mesh->namein,".meshb");
    if ( ptr )  strcat(mesh->nameout,"b");
  }

  if ( met->namein == NULL ) {
    ptr = strstr(mesh->namein,".mesh");
    if ( ptr )
      SAFE_CALLOC(met->namein,(strlen(mesh->namein)+1),char);
    else
      SAFE_CALLOC(met->namein,(strlen(mesh->namein)+5),char);

    strcpy(met->namein,mesh->namein);
    ptr = strstr(met->namein,".mesh");
    if ( ptr ) *ptr = '\0';
    strcat(met->namein,".sol");
    ADD_MEM(mesh,(strlen(met->namein)+1)*sizeof(char),"input sol name",
            printf("  Exit program.\n");
            exit(EXIT_FAILURE));
    SAFE_REALLOC(met->namein,(strlen(met->namein)+1),char);
  }
  if ( met->nameout == NULL ) {
    ptr = strstr(mesh->nameout,".mesh");
    if ( ptr )
      SAFE_CALLOC(met->nameout,strlen(mesh->nameout)+1,char);
    else
      SAFE_CALLOC(met->nameout,strlen(mesh->nameout)+5,char);
    strcpy(met->nameout,mesh->nameout);
    ptr = strstr(met->nameout,".mesh");
    if ( ptr ) *ptr = '\0';
    strcat(met->nameout,".sol");
    ADD_MEM(mesh,(strlen(met->nameout)+1)*sizeof(char),"output sol name",
            printf("  Exit program.\n");
            exit(EXIT_FAILURE));
    SAFE_REALLOC(met->nameout,(strlen(met->nameout)+1),char);
  }
  return(1);
}

/** Read parammeter file */
static inline
int parsop(pMesh mesh,pSol met) {
  Par        *par;
  float       fp1;
  int         i,j,ret;
  char       *ptr,buf[256],data[256];
  FILE       *in;

  /* check for parameter file */
  strcpy(data,mesh->namein);
  ptr = strstr(data,".mesh");
  if ( ptr )  *ptr = '\0';
  strcat(data,".mmg3d5");
  in = fopen(data,"r");
  if ( !in ) {
    sprintf(data,"%s","DEFAULT.mmg3d5");
    in = fopen(data,"r");
    if ( !in )  return(1);
  }
  fprintf(stdout,"  %%%% %s OPENED\n",data);

  /* read parameters */
  while ( !feof(in) ) {
    /* scan line */
    ret = fscanf(in,"%s",data);
    if ( !ret || feof(in) )  break;
    for (i=0; i<strlen(data); i++) data[i] = tolower(data[i]);

    /* check for condition type */
    if ( !strcmp(data,"parameters") ) {
      fscanf(in,"%d",&mesh->info.npar);
      ADD_MEM(mesh,mesh->info.npar*sizeof(Par),"parameters",
              printf("  Exit program.\n");
              exit(EXIT_FAILURE));
      SAFE_CALLOC(mesh->info.par,mesh->info.npar,Par);

      for (i=0; i<mesh->info.npar; i++) {
        par = &mesh->info.par[i];
        fscanf(in,"%d %s ",&par->ref,buf);
        for (j=0; j<strlen(buf); j++)  buf[j] = tolower(buf[j]);
        if ( !strcmp(buf,"triangles") || !strcmp(buf,"triangle") )
          par->elt = MMG5_Triangle;
        else {
          fprintf(stdout,"  %%%% Wrong format: %s\n",buf);
          continue;
        }
        ret = fscanf(in,"%f",&fp1);
        par->hausd = fp1;
      }
    }
  }
  fclose(in);
  return(1);
}

/** Deallocations before return */
void Free_all(pMesh mesh,pSol met
#ifdef SINGUL
             ,pSingul singul
#endif
             ){

#ifdef SINGUL
  Free_structures(mesh,met,singul);
#else
  Free_structures(mesh,met);
#endif

}

/** set pointer for MMG5_saveMesh function */
static inline
void Set_saveFunc(pMesh mesh) {
  MMG5_saveMesh = saveMesh;
}

static void endcod() {
  char    stim[32];

  chrono(OFF,&MMG5_ctim[0]);
  printim(MMG5_ctim[0].gdif,stim);
  fprintf(stdout,"\n   ELAPSED TIME  %s\n",stim);
}

/** main programm */
int main(int argc,char *argv[]) {
  Mesh      mesh;
  Sol       met;
  Singul    sing;
  int       ier;
  char      stim[32];

  fprintf(stdout,"  -- MMG3d, Release %s (%s) \n",MG_VER,MG_REL);
  fprintf(stdout,"     %s\n",MG_CPY);
  fprintf(stdout,"    %s %s\n",__DATE__,__TIME__);

  signal(SIGABRT,excfun);
  signal(SIGFPE,excfun);
  signal(SIGILL,excfun);
  signal(SIGSEGV,excfun);
  signal(SIGTERM,excfun);
  signal(SIGINT,excfun);
  atexit(endcod);

  tminit(MMG5_ctim,TIMEMAX);
  chrono(ON,&MMG5_ctim[0]);

  /* assign default values */
  memset(&mesh,0,sizeof(Mesh));
  memset(&met,0,sizeof(Sol));
#ifdef SINGUL
  memset(&sing,0,sizeof(Singul));
#endif

  Init_parameters(&mesh);

  met.size      = 1;

  /* command line */
  if ( !parsar(argc,argv,&mesh,&met,&sing) )  return(MMG5_STRONGFAILURE);
#ifdef USE_SCOTCH
  warnScotch(&mesh);
#endif

  /* load data */
  fprintf(stdout,"\n  -- INPUT DATA\n");
  chrono(ON,&MMG5_ctim[1]);
  warnOrientation(&mesh);
  /* read mesh file */
  if ( !loadMesh(&mesh) ) RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);

  /* read metric if any */
  ier = loadMet(&mesh,&met);
  if ( !ier )
    RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
  else if ( ier > 0 && met.np != mesh.np ) {
    fprintf(stdout,"  ## WARNING: WRONG SOLUTION NUMBER. IGNORED\n");
    DEL_MEM(&mesh,met.m,(met.size*met.npmax+1)*sizeof(double));
    met.np = 0;
  } else if ( met.size!=1 ) {
    fprintf(stdout,"  ## ERROR: ANISOTROPIC METRIC NOT IMPLEMENTED.\n");
    RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
  }
#ifdef SINGUL
  if ( mesh.info.sing ) {
    if ( !mesh.info.iso ) {
      if ( !sing.namein )
        fprintf(stdout,"  ## WARNING: NO SINGULARITIES PROVIDED.\n");
      else
        if ( !loadSingul(&mesh,&sing) )
          RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
    }
    else if ( sing.namein ) {
      fprintf(stdout,"  ## WARNING: SINGULARITIES MUST BE INSERTED IN");
      fprintf(stdout," A PRE-REMESHING PROCESS.\n");
      fprintf(stdout,"              FILE %s IGNORED\n",sing.namein);
    }
  }
#endif
  if ( !parsop(&mesh,&met) )
    RETURN_AND_FREE(&mesh,&met,&sing,MMG5_LOWFAILURE);

  chrono(OFF,&MMG5_ctim[1]);
  printim(MMG5_ctim[1].gdif,stim);
  fprintf(stdout,"  -- DATA READING COMPLETED.     %s\n",stim);

  /* analysis */
  chrono(ON,&MMG5_ctim[2]);
  setfunc(&mesh,&met);
  Set_saveFunc(&mesh);

  if ( abs(mesh.info.imprim) > 0 )  outqua(&mesh,&met);
  fprintf(stdout,"\n  %s\n   MODULE MMG3D: IMB-LJLL : %s (%s)\n  %s\n",
          MG_STR,MG_VER,MG_REL,MG_STR);
  if ( mesh.info.imprim )  fprintf(stdout,"\n  -- PHASE 1 : ANALYSIS\n");

  if ( !scaleMesh(&mesh,&met,&sing) )
    RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
  if ( mesh.info.iso ) {
    if ( !met.np ) {
      fprintf(stdout,"\n  ## ERROR: A VALID SOLUTION FILE IS NEEDED \n");
      RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
    }
    if ( !mmg3d2(&mesh,&met) )
      RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
  }

#ifdef SINGUL
  if ( mesh.info.sing ) {
    if ( !mesh.info.iso ) {
      if ( !met.np && !DoSol(&mesh,&met) )
        RETURN_AND_FREE(&mesh,&met,&sing,MMG5_LOWFAILURE);
      if ( !( ier=inserSingul(&mesh,&met,&sing) ) )
        RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
      else if (ier > 0 ) {
        chrono(OFF,&MMG5_ctim[2]);
        printim(MMG5_ctim[2].gdif,stim);
        fprintf(stdout,"  -- INSERTION OF SINGULARITIES COMPLETED.     %s\n\n",stim);
        chrono(ON,&MMG5_ctim[2]);
      }
    }
  }
#endif

#ifdef DEBUG
  if ( !met.np && !DoSol(&mesh,&met,&mesh.info) )
    RETURN_AND_FREE(&mesh,&met,&sing,MMG5_LOWFAILURE);
#endif

  if ( !analys(&mesh) ) RETURN_AND_FREE(&mesh,&met,&sing,MMG5_LOWFAILURE);

  if ( mesh.info.imprim > 4 && !mesh.info.iso && met.m ) prilen(&mesh,&met);

  chrono(OFF,&MMG5_ctim[2]);
  printim(MMG5_ctim[2].gdif,stim);
  if ( mesh.info.imprim )
    fprintf(stdout,"  -- PHASE 1 COMPLETED.     %s\n",stim);

  /* mesh adaptation */
  chrono(ON,&MMG5_ctim[3]);
  if ( mesh.info.imprim )
    fprintf(stdout,"\n  -- PHASE 2 : %s MESHING\n",met.size < 6 ? "ISOTROPIC" : "ANISOTROPIC");

#ifdef SINGUL
  if ( mesh.info.sing && (!mesh.info.iso) ) {
    if ( colSing(&mesh,&met)<0 ) {
      fprintf(stdout,"  ## Collapse of singularities problem.\n");
      // RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
    }
  }
#endif

  if ( !mmg3d1(&mesh,&met) ){
    if ( !(mesh.adja) && !hashTetra(&mesh,1) ) {
      fprintf(stdout,"  ## Hashing problem. Unable to save mesh.\n");
      RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
    }
    if ( !unscaleMesh(&mesh,&met) )
      RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
    if ( !saveMesh(&mesh) )
      RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
    if ( met.m && !saveMet(&mesh,&met) )
      RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
    RETURN_AND_FREE(&mesh,&met,&sing,MMG5_LOWFAILURE);
  }

#ifdef SINGUL
  if ( mesh.info.sing && (!mesh.info.iso) ) {
    if ( !solveUnsignedTet(&mesh,&met) ) {
      fprintf(stdout,"  ## Solve of undetermined tetrahedra problem.\n");
      if ( !unscaleMesh(&mesh,&met) )
        RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
      if ( !saveMesh(&mesh) )
        RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
      if ( met.m && !saveMet(&mesh,&met) )
        RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
      RETURN_AND_FREE(&mesh,&met,&sing,MMG5_LOWFAILURE);
    }
  }
#endif

  chrono(OFF,&MMG5_ctim[3]);
  printim(MMG5_ctim[3].gdif,stim);
  if ( mesh.info.imprim )
    fprintf(stdout,"  -- PHASE 2 COMPLETED.     %s\n",stim);
  fprintf(stdout,"\n  %s\n   END OF MODULE MMG3d: IMB-LJLL \n  %s\n",MG_STR,MG_STR);

  /* save file */
  outqua(&mesh,&met);

  if ( mesh.info.imprim > 4 && !mesh.info.iso )
    prilen(&mesh,&met);

  chrono(ON,&MMG5_ctim[1]);
  if ( mesh.info.imprim )  fprintf(stdout,"\n  -- WRITING DATA FILE %s\n",mesh.nameout);
  if ( !unscaleMesh(&mesh,&met) )
    RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);

  if ( !saveMesh(&mesh) )
    RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);

  if ( !saveMet(&mesh,&met) )
    RETURN_AND_FREE(&mesh,&met,&sing,MMG5_STRONGFAILURE);
  chrono(OFF,&MMG5_ctim[1]);
  if ( mesh.info.imprim )  fprintf(stdout,"  -- WRITING COMPLETED\n");

  /* free mem */
  RETURN_AND_FREE(&mesh,&met,&sing,MMG5_SUCCESS);
}
