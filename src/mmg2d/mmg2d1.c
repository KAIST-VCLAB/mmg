/* =============================================================================
**  This file is part of the mmg software package for the tetrahedral
**  mesh modification.
**  Copyright (c) Bx INP/Inria/UBordeaux/UPMC, 2004- .
**
**  mmg is free software: you can redistribute it and/or modify it
**  under the terms of the GNU Lesser General Public License as published
**  by the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  mmg is distributed in the hope that it will be useful, but WITHOUT
**  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
**  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
**  License for more details.
**
**  You should have received a copy of the GNU Lesser General Public
**  License and of the GNU General Public License along with mmg (in
**  files COPYING.LESSER and COPYING). If not, see
**  <http://www.gnu.org/licenses/>. Please read their terms carefully and
**  use this copy of the mmg distribution only if you accept them.
** =============================================================================
*/
/**
 * \file mmg2d/mmg2d1.c
 * \brief Mesh adaptation functions.
 * \author Charles Dapogny (UPMC)
 * \author Cécile Dobrzynski (Bx INP/Inria/UBordeaux)
 * \author Pascal Frey (UPMC)
 * \author Algiane Froehly (Inria/UBordeaux)
 * \version 5
 * \copyright GNU Lesser General Public License.
 */
#include "mmg2d.h"

unsigned char ddb;

/* Mesh adaptation routine for the first stages of the algorithm: intertwine splitting
 based on patterns, collapses and swaps.
   typchk = 1 -> adaptation based on edge lengths
   typchk = 2 -> adaptation based on lengths calculated in metric met */
int _MMG2_anatri(MMG5_pMesh mesh,MMG5_pSol met,char typchk) {
  int      it,maxit,ns,nc,nsw,nns,nnc,nnsw;

  nns = nnc = nnsw = 0;
  ns = nc = nsw = 0;
  it = 0;
  maxit = 5;

  /* Main routine; intertwine split, collapse and swaps */
  do {
    if ( !mesh->info.noinsert ) {
      /* Memory free */
      _MMG5_DEL_MEM(mesh,mesh->adja,(3*mesh->ntmax+5)*sizeof(int));
      mesh->adja = 0;

      /* Split long edges according to patterns */
      ns = _MMG2_anaelt(mesh,met,typchk);
      if ( ns < 0 ) {
        fprintf(stdout,"  ## Unable to complete surface mesh. Exit program.\n");
        return(0);
      }

      /* Recreate adjacencies */
      if ( !MMG2_hashTria(mesh) ) {
        fprintf(stdout,"  ## Hashing problem. Exit program.\n");
        return(0);
      }

      /*if ( it==4 ){
        printf("Saving mesh...\n");
        if ( !MMG2_hashTria(mesh) ) {
          fprintf(stdout,"  ## Hashing problem. Exit program.\n");
          return(0);
        }

        MMG2_bdryEdge(mesh);
        _MMG2_savemesh_db(mesh,mesh->nameout,0);
        _MMG2_savemet_db(mesh,met,mesh->nameout,0);
        exit(EXIT_FAILURE);
      }*/

      /* Collapse short edges */
      nc = _MMG2_colelt(mesh,met,typchk);
      if ( nc < 0 ) {
        fprintf(stdout,"  ## Unable to collapse mesh. Exiting.\n");
        return(0);
      }

    }
    else {
      ns = 0;
      nc = 0;
    }

    /* Swap edges */
    if ( !mesh->info.noswap ) {
      nsw = _MMG2_swpmsh(mesh,met,typchk);
      if ( nsw < 0 ) {
        fprintf(stdout,"  ## Unable to improve mesh. Exiting.\n");
        return(0);
      }
    }
    else nsw = 0;

    nns += ns;
    nnc += nc;
    nnsw += nsw;

    if ( (abs(mesh->info.imprim) > 4 || mesh->info.ddebug) && ns+nc > 0 )
      fprintf(stdout,"     %8d splitted, %8d collapsed, %8d swapped\n",ns,nc,nsw);
    if ( it > 3 && abs(nc-ns) < 0.1 * MG_MAX(nc,ns) )  break;
  }
  while ( ++it < maxit && ns+nc+nsw >0 );

  if ( mesh->info.imprim ) {
    if ( (abs(mesh->info.imprim) < 5 || mesh->info.ddebug ) && nns+nnc > 0 )
      fprintf(stdout,"     %8d splitted, %8d collapsed, %8d swapped, %d iter.\n",nns,nnc,nnsw,it);
  }
  return(1);
}

/* Travel triangles and split long edges according to patterns */
int _MMG2_anaelt(MMG5_pMesh mesh,MMG5_pSol met,int typchk) {
  MMG5_pTria      pt;
  MMG5_pPoint     ppt,p1,p2;
  _MMG5_Hash      hash;
  double          len,s,o[2],no[2];
  int             ns,nc,npinit,ni,k,nt,ip1,ip2,ip,it,vx[3];
  char            i,ic,i1,i2,ier;

  s = 0.5;
  ns = 0;
  npinit = mesh->np;

  if ( !_MMG5_hashNew(mesh,&hash,mesh->np,3*mesh->np) ) return(0);

  /* Step 1: travel mesh, check edges, and tags those to be split; create the new vertices in hash */
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) || (pt->ref < 0) ) continue;
    if ( MG_SIN(pt->tag[0]) || MG_SIN(pt->tag[1]) || MG_SIN(pt->tag[2]) )  continue;

    /* Check if pt should be cut */
    pt->flag = 0;

    /* typchk=1 : split on geometric basis and rough size considerations */
    if ( typchk == 1) {
      if ( !_MMG2_chkedg(mesh,k) ) continue;
    }
    /* typchk =2 : split on basis of edge lengths in the metric */
    else if ( typchk ==2 ) {
      for (i=0; i<3; i++) {
        i1 = _MMG5_inxt2[i];
        i2 = _MMG5_iprv2[i];
        len = MMG2D_lencurv(mesh,met,pt->v[i1],pt->v[i2]);
        if ( len > MMG2_LLONG ) MG_SET(pt->flag,i);
      }
    }
    if ( !pt->flag ) continue;
    ns++;

    /* Travel edges to split and create new points */
    for (i=0; i<3; i++) {
      if ( !MG_GET(pt->flag,i) ) continue;
      i1  = _MMG5_inxt2[i];
      i2  = _MMG5_iprv2[i];
      ip1 = pt->v[i1];
      ip2 = pt->v[i2];
      ip = _MMG5_hashGet(&hash,ip1,ip2);
      if ( ip > 0 ) continue;

      /* Geometric attributes of the new point */
      ier = _MMG2_bezierCurv(mesh,k,i,s,o,no);
      if ( !ier ) {
        MG_CLR(pt->flag,i);
        continue;
      }
      ip = _MMG2D_newPt(mesh,o,pt->tag[i]);
      if ( !ip ) {
        /* reallocation of point table */
        _MMG2D_POINT_REALLOC(mesh,met,ip,mesh->gap,
                            printf("  ## Error: unable to allocate a new point.\n");
                            _MMG5_INCREASE_MEM_MESSAGE();
                            do {
                              _MMG2D_delPt(mesh,mesh->np);
                            } while ( mesh->np>npinit );
                            return(-1)
                            ,o,pt->tag[i]);
      }
      ppt = &mesh->point[ip];
      if ( MG_EDG(pt->tag[i]) ) {
        ppt->n[0] = no[0];
        ppt->n[1] = no[1];
      }

      /* If there is a metric in the mesh, interpolate it at the new point */
      if ( met->m )
        MMG2D_intmet(mesh,met,k,i,ip,s);

      /* Add point to the hashing structure */
      _MMG5_hashEdge(mesh,&hash,ip1,ip2,ip);
    }
  }
  if ( !ns ) {
    _MMG5_DEL_MEM(mesh,hash.item,(hash.max+1)*sizeof(_MMG5_hedge));
    return(ns);
  }

  /* Step 2: Make flags at triangles consistent between themselves (check if adjacent triangle is split) */
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) || pt->ref < 0 ) continue;
    else if ( pt->flag == 7 ) continue;
    nc = 0;

    for (i=0; i<3; i++) {
      i1 = _MMG5_iprv2[i];
      i2 = _MMG5_inxt2[i];
      if ( !MG_GET(pt->flag,i) && !MG_SIN(pt->tag[i]) ) {
        ip = _MMG5_hashGet(&hash,pt->v[i1],pt->v[i2]);
        if ( ip > 0 ) {
          MG_SET(pt->flag,i);
          nc++;
        }
      }
    }
    if ( nc > 0 ) ns++;
  }
  if ( mesh->info.ddebug && ns ) {
    fprintf(stdout,"     %d analyzed  %d proposed\n",mesh->nt,ns);
    fflush(stdout);
  }

  /* Step 3: Simulate splitting and delete points leading to an invalid configuration */
  for (k=1; k<=mesh->np; k++)
    mesh->point[k].flag = 0;

  it = 1;
  nc = 0;
  do {
    ni = 0;
    for ( k=1; k<= mesh->nt; k++) {
      pt = &mesh->tria[k];
      if ( !MG_EOK(pt) || pt->ref < 0 ) continue;
      else if ( pt->flag == 0 ) continue;

      vx[0] = vx[1] =vx[2] = 0;
      pt->flag = 0;
      ic = 0;

      for (i=0; i<3; i++) {
        i1 = _MMG5_iprv2[i];
        i2 = _MMG5_inxt2[i];
        vx[i] = _MMG5_hashGet(&hash,pt->v[i1],pt->v[i2]);
        if ( vx[i] > 0 ) {
          MG_SET(pt->flag,i);
          if ( mesh->point[vx[i]].flag > 2 )  ic = 1;
        }
      }

      if ( !pt->flag )  continue;
      switch (pt->flag) {
        case 1: case 2: case 4:
          ier = _MMG2_split1_sim(mesh,met,k,vx);
          break;
        case 7:
          ier = _MMG2_split3_sim(mesh,met,k,vx);
          break;
        default:
          ier = _MMG2_split2_sim(mesh,met,k,vx);
          break;
      }
      if ( ier )  continue;

      /* An edge is invalidated in the process */
      ni++;
      if ( ic == 0 && _MMG2_dichoto(mesh,met,k,vx) ) {
        for (i=0; i<3; i++)
          if ( vx[i] > 0 )  mesh->point[vx[i]].flag++;
      }
      /* Relocate point at the center of the edge */
      else {
        for (i=0; i<3; i++) {
          if ( vx[i] > 0 ) {
            p1 = &mesh->point[pt->v[_MMG5_iprv2[i]]];
            p2 = &mesh->point[pt->v[_MMG5_inxt2[i]]];
            ppt = &mesh->point[vx[i]];
            ppt->c[0] = 0.5 * (p1->c[0] + p2->c[0]);
            ppt->c[1] = 0.5 * (p1->c[1] + p2->c[1]);
          }
        }
      }
    }
    nc += ni;
  }
  while ( ni >0 && ++it <20 );

  if ( mesh->info.ddebug && nc ) {
    fprintf(stdout,"     %d corrected,  %d invalid\n",nc,ni);
    fflush(stdout);
  }

  /* step 4: effective splitting */
  ns = 0;
  nt = mesh->nt;
  for (k=1; k<=nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) || pt->ref < 0 )  continue;
    else if ( pt->flag == 0 )  continue;

    vx[0] = vx[1] = vx[2] = 0;
    for (i=0; i<3; i++) {
      i1 = _MMG5_inxt2[i];
      i2 = _MMG5_inxt2[i1];
      if ( MG_GET(pt->flag,i) ) {
        vx[i] = _MMG5_hashGet(&hash,pt->v[i1],pt->v[i2]);
        if ( !vx[i] ) {
          printf("Error: unable to create point on edge.\n Exit program.\n");
          exit(EXIT_FAILURE);
        }
      }
    }
    if ( pt->flag == 1 || pt->flag == 2 || pt->flag == 4 ) {
      ier = _MMG2_split1(mesh,met,k,vx);
      assert(ier);
      ns++;
    }
    else if ( pt->flag == 7 ) {
      ier = _MMG2_split3(mesh,met,k,vx);
      assert(ier);
      ns++;
    }
    else {
      ier = _MMG2_split2(mesh,met,k,vx);
      assert(ier);
      ns++;
    }
  }
  if ( (mesh->info.ddebug || abs(mesh->info.imprim) > 5) && ns > 0 )
    fprintf(stdout,"     %7d splitted\n",ns);
  _MMG5_DEL_MEM(mesh,hash.item,(hash.max+1)*sizeof(_MMG5_hedge));

  return(ns);
}

/**
 * \param mesh pointer toward the mesh structure.
 * \param met pointer toward the metric structure.
 * \param k element index.
 * \param vx pointer toward table of edges to split.
 * \return 1.
 *
 * Find acceptable position for splitting.
 *
 */
int _MMG2_dichoto(MMG5_pMesh mesh,MMG5_pSol met,int k,int *vx) {
  MMG5_pTria   pt;
  MMG5_pPoint  pa,pb,ps;
  double       o[3][2],p[3][2];
  float        to,tp,t;
  int          ia,ib,ier,it,maxit;
  char         i,i1,i2,j;

  pt = &mesh->tria[k];

  /* Get point on curve and along segment for edge split */
  for (i=0; i<3; i++) {
    memset(p[i],0,2*sizeof(double));
    memset(o[i],0,2*sizeof(double));
    if ( vx[i] > 0 ) {
      i1 = _MMG5_inxt2[i];
      i2 = _MMG5_inxt2[i1];
      ia = pt->v[i1];
      ib = pt->v[i2];
      pa = &mesh->point[ia];
      pb = &mesh->point[ib];
      ps = &mesh->point[vx[i]];
      o[i][0] = 0.5 * (pa->c[0] + pb->c[0]);
      o[i][1] = 0.5 * (pa->c[1] + pb->c[1]);
      p[i][0] = ps->c[0];
      p[i][1] = ps->c[1];
    }
  }
  maxit = 4;
  it = 0;
  tp = 1.0;
  to = 0.0;

  do {
    /* compute new position */
    t = 0.5 * (tp + to);
    for (i=0; i<3; i++) {
      if ( vx[i] > 0 ) {
        ps = &mesh->point[vx[i]];
        ps->c[0] = o[i][0] + t*(p[i][0] - o[i][0]);
        ps->c[1] = o[i][1] + t*(p[i][1] - o[i][1]);
        j=i;
      }
    }
    switch (pt->flag) {
      case 1: case 2: case 4:
        ier = _MMG2_split1_sim(mesh,met,k,vx);
        break;
      case 7:
        ier = _MMG2_split3_sim(mesh,met,k,vx);
        break;
      default:
        ier = _MMG2_split2_sim(mesh,met,k,vx);
        break;
    }
    if ( ier )
      to = t;
    else
      tp = t;
  }
  while ( ++it < maxit );

  /* restore coords of last valid pos. */
  if ( !ier ) {
    t = to;
    for (i=0; i<3; i++) {
      if ( vx[i] > 0 ) {
        ps = &mesh->point[vx[i]];
        ps->c[0] = o[i][0] + t*(p[i][0] - o[i][0]);
        ps->c[1] = o[i][1] + t*(p[i][1] - o[i][1]);
      }
    }
  }
  return(1);
}

/* Travel triangles and collapse short edges */
int _MMG2_colelt(MMG5_pMesh mesh,MMG5_pSol met,int typchk) {
  MMG5_pTria         pt;
  MMG5_pPoint        p1,p2;
  double             ux,uy,ll,hmin2;
  int                list[MMG2_LONMAX+2],ilist,nc,k;
  unsigned char      i,i1,i2,open;

  nc = 0;
  hmin2 = mesh->info.hmin * mesh->info.hmin;

  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) || pt->ref < 0 ) continue;

    /* Travel 3 edges of the triangle and decide whether to collapse p1->p2, based on length criterion */
    pt->flag = 0; // was here before, but probably serves for nothing

    for (i=0; i<3; i++) {
      if ( MG_SIN(pt->tag[i]) ) continue;
      i1 = _MMG5_inxt2[i];
      i2 = _MMG5_iprv2[i];
      p1 = &mesh->point[pt->v[i1]];
      p2 = &mesh->point[pt->v[i2]];
      if ( MG_SIN(p1->tag) ) continue;

      /* Impossible to collapse a surface point onto a non surface point -- impossible to
       collapse a surface point along a non geometric edge */
      else if ( p1->tag & MG_GEO ) {
        if ( ! (p2->tag & MG_GEO) || !(pt->tag[i] & MG_GEO) ) continue;
      }

      open = (mesh->adja[3*(k-1)+1+i] == 0) ? 1 : 0;

      /* Check length */
      if ( typchk == 1 ) {
        ux = p2->c[0] - p1->c[0];
        uy = p2->c[1] - p1->c[1];
        ll = ux*ux + uy*uy;
        if ( ll > hmin2 ) continue;
      }
      else {
        ll = MMG2D_lencurv(mesh,met,pt->v[i1],pt->v[i2]);
        if ( ll > MMG2_LSHRT ) continue;
      }

      /* Check whether the geometry is preserved */
      ilist = _MMG2_chkcol(mesh,met,k,i,list,typchk);

      if ( ilist > 3 || ( ilist == 3 && open ) ) {
        nc += _MMG2_colver(mesh,ilist,list);
        break;
      }
      else if ( ilist == 3 ) {
        nc += _MMG2_colver3(mesh,list);
        break;
      }
      else if ( ilist == 2 ) {
        nc += _MMG2_colver2(mesh,list);
        break;
      }
    }
  }

  if ( nc > 0 && (abs(mesh->info.imprim) > 5 || mesh->info.ddebug) )
    fprintf(stdout,"     %8d vertices removed\n",nc);

  return(nc);
}

/* Travel triangles and swap edges to improve quality */
int _MMG2_swpmsh(MMG5_pMesh mesh,MMG5_pSol met,int typchk) {
  MMG5_pTria          pt;
  int                 it,maxit,ns,nns,k;
  unsigned char       i;

  it = nns = 0;
  maxit = 2;
  mesh->base++;

  do {
    ns = 0;
    for (k=1; k<=mesh->nt; k++) {
      pt = &mesh->tria[k];
      if ( !MG_EOK(pt) || pt->ref < 0 ) continue;

      for (i=0; i<3; i++) {
        if ( MG_SIN(pt->tag[i]) || MG_EDG(pt->tag[i]) ) continue;
        else if ( _MMG2_chkswp(mesh,met,k,i,typchk) ) {
          ns += _MMG2_swapar(mesh,k,i);
          break;
        }
      }
    }
    nns += ns;
  }
  while ( ns > 0 && ++it<maxit );
  if ( (abs(mesh->info.imprim) > 5 || mesh->info.ddebug) && nns > 0 )
    fprintf(stdout,"     %8d edge swapped\n",nns);

  return(nns);
}


/* Mesh adaptation routine for the final stage of the algorithm: intertwine splitting
 based on patterns, collapses, swaps and vertex relocations.*/
int _MMG2_adptri(MMG5_pMesh mesh,MMG5_pSol met) {
  int                  maxit,it,nns,ns,nnc,nc,nnsw,nsw,nnm,nm;

  nns = nnc = nnsw = nnm = it = 0;
  maxit = 10;

  do {
    _MMG2_chkmsh(mesh);
    if ( !mesh->info.noinsert ) {
      ns = _MMG2_adpspl(mesh,met);
      if ( ns < 0 ) {
        fprintf(stdout,"  ## Problem in function adpspl. Unable to complete mesh. Exit program.\n");
        return(0);
      }

      nc = _MMG2_adpcol(mesh,met);
      if ( nc < 0 ) {
        fprintf(stdout,"  ## Problem in function adpcol. Unable to complete mesh. Exit program.\n");
        return(0);
      }
    }
    else {
      ns = 0;
      nc = 0;
    }

    if ( !mesh->info.noswap ) {
      nsw = _MMG2_swpmsh(mesh,met,2);
      if ( nsw < 0 ) {
        fprintf(stdout,"  ## Problem in function swpmsh. Unable to complete mesh. Exit program.\n");
        return(0);
      }
    }
    else
      nsw = 0;

    if ( !mesh->info.nomove ) {
      nm = _MMG2_movtri(mesh,met,1,0);
      if ( nm < 0 ) {
        fprintf(stdout,"  ## Problem in function movtri. Unable to complete mesh. Exit program.\n");
        return(0);
      }
    }
    else
      nm = 0;

    nns  += ns;
    nnc  += nc;
    nnsw += nsw;
    nnm  += nm;

    if ( (abs(mesh->info.imprim) > 4 || mesh->info.ddebug) && ns+nc+nsw+nm > 0 )
      fprintf(stdout,"     %8d splitted, %8d collapsed, %8d swapped, %8d moved\n",ns,nc,nsw,nm);
    if ( ns < 10 && abs(nc-ns) < 3 )  break;
    else if ( it > 3 && abs(nc-ns) < 0.3 * MG_MAX(nc,ns) )  break;
  }
  while( ++it < maxit && (nc+ns+nsw+nm > 0) );

  /* Last iterations of vertex relocation only */
  /*if ( !mesh->info.nomove ) {
    nm = _MMG2_movtri(mesh,met,5,1);
    if ( nm < 0 ) {
      fprintf(stdout,"  ## Problem in function movtri. Unable to complete mesh. Exit program.\n");
      return(0);
    }
    nnm += nm;
  }*/

  if ( mesh->info.imprim ) {
    if ( abs(mesh->info.imprim) < 5 && (nnc > 0 || nns > 0) )
      fprintf(stdout,"     %8d splitted, %8d collapsed, %8d swapped, %8d moved, %d iter. \n",nns,nnc,nnsw,nnm,it);
  }
  return(1);
}

/* Analysis and splitting routine for edges in the final step of the algorithm; edges are only splitted on
 a one-by-one basis */
int _MMG2_adpspl(MMG5_pMesh mesh,MMG5_pSol met) {
  MMG5_pTria         pt;
  MMG5_pPoint        p1,p2;
  double             lmax,len;
  int                k,ns,ip1,ip2,ip,ier;
  char               i,i1,i2,imax;

  ns = 0;

  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) || pt->ref < 0 ) continue;

    imax = -1;
    lmax = -1.0;
    for (i=0; i<3; i++) {
      i1 = _MMG5_inxt2[i];
      i2 = _MMG5_iprv2[i];
      len = MMG2D_lencurv(mesh,met,pt->v[i1],pt->v[i2]);

      if ( len > lmax ) {
        lmax = len;
        imax = i;
      }
    }

    if ( lmax < MMG2_LOPTL ) continue;
    else if ( MG_SIN(pt->tag[imax]) ) continue;

    /* Check the feasibility of splitting */
    i1 = _MMG5_inxt2[imax];
    i2 = _MMG5_iprv2[imax];
    p1 = &mesh->point[pt->v[i1]];
    p2 = &mesh->point[pt->v[i2]];

    ip = _MMG2_chkspl(mesh,met,k,imax);

    /* Lack of memory; abort the routine */
    if ( ip < 0 ){
      return(ns);
    }
    else if ( ip > 0 ) {
      ier = _MMG2_split1b(mesh,k,imax,ip);

      /* Lack of memory; abort the routine */
      if ( !ier ) {
        _MMG2D_delPt(mesh,ip);
        return(ns);
      }

      /* if we realloc memory in split1b pt pointer is not valid aymore. */
      pt = &mesh->tria[k];
      ns += ier;
    }
  }

  return(ns);
}

/* Analysis and collapse routine for edges in the final step of the algorithm */
int _MMG2_adpcol(MMG5_pMesh mesh,MMG5_pSol met) {
  MMG5_pTria        pt;
  MMG5_pPoint       p1,p2;
  double            len;
  int               k,nc,ilist,list[MMG2_LONMAX+2];
  char              i,i1,i2,open;

  nc = 0;
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) || pt->ref < 0 ) continue;

    /* Check edge length, and attempt collapse */
    pt->flag = 0;
    for (i=0; i<3; i++) {
      if ( MG_SIN(pt->tag[i]) ) continue;

      open = ( mesh->adja[3*(k-1)+1+i] == 0 ) ? 1 : 0;

      i1 = _MMG5_inxt2[i];
      i2 = _MMG5_iprv2[i];
      p1 = &mesh->point[pt->v[i1]];
      p2 = &mesh->point[pt->v[i2]];

      if ( MG_SIN(p1->tag) ) continue;
      else if ( p1->tag & MG_GEO ) {
        if ( ! (p2->tag & MG_GEO) || !(pt->tag[i] & MG_GEO) ) continue;
      }

      len = MMG2D_lencurv(mesh,met,pt->v[i1],pt->v[i2]);

      if ( len > MMG2_LOPTS ) continue;

      ilist = _MMG2_chkcol(mesh,met,k,i,list,2);
      if ( ilist > 3 || ( ilist==3 && open ) ) {
        nc += _MMG2_colver(mesh,ilist,list);
        break;
      }
      else if ( ilist == 3 ) {
        nc += _MMG2_colver3(mesh,list);
        break;
      }
      else if ( ilist == 2 ) {
        nc += _MMG2_colver2(mesh,list);
        break;
      }
    }
  }

  return(nc);
}

/* Analyze points to relocate them according to a quality criterion */
int _MMG2_movtri(MMG5_pMesh mesh,MMG5_pSol met,int maxit,char improve) {
  MMG5_pTria           pt;
  MMG5_pPoint          p0;
  int                  base,k,nnm,nm,ns,it,ilist,list[MMG2_LONMAX+2];
  char                 i,ier;

  it = nnm = 0;
  base = 0;

  for (k=1; k<=mesh->np; k++)
    mesh->point[k].flag = base;

  do {
    base++;
    nm = ns = 0;
    for (k=1; k<=mesh->nt; k++) {
      pt = &mesh->tria[k];
      if ( !MG_EOK(pt) || pt->ref < 0 ) continue;

      for (i=0; i<3; i++) {
        p0 = &mesh->point[pt->v[i]];
        if ( p0->flag == base || MG_SIN(p0->tag) || p0->tag & MG_NOM ) continue;
        ier = 0;

        ilist = _MMG2_boulet(mesh,k,i,list);

        if ( MG_EDG(p0->tag) ) {
          ier = _MMG2_movedgpt(mesh,met,ilist,list,improve);
          if ( ier ) ns++;
        }
        else
          ier = _MMG2_movintpt(mesh,met,ilist,list,improve);

        if ( ier ) {
          nm++;
          p0->flag = base;
        }
      }
    }
    nnm += nm;
    if ( mesh->info.ddebug )  fprintf(stdout,"     %8d moved, %d geometry\n",nm,ns);
  }
  while ( ++it < maxit && nm > 0 );

  if ( (abs(mesh->info.imprim) > 5 || mesh->info.ddebug) && nnm > 0 )
    fprintf(stdout,"     %8d vertices moved, %d iter.\n",nnm,it);

  return(nnm);
}

/**
 * \param mesh pointer toward the mesh structure.
 * \param sol pointer toward the sol structure.
 * \return 1 if success, 0 if strongly fail.
 *
 * Mesh adaptation -- new version of mmg2d1.c
 *
 **/
int MMG2_mmg2d1n(MMG5_pMesh mesh,MMG5_pSol met) {

  /* Stage 1: creation of a geometric mesh */
  if ( abs(mesh->info.imprim) > 4 || mesh->info.ddebug )
    fprintf(stdout,"  ** GEOMETRIC MESH\n");

  if ( !_MMG2_anatri(mesh,met,1) ) {
    fprintf(stdout,"  ## Unable to split mesh-> Exiting.\n");
    return(0);
  }

  /* Stage 2: creation of a computational mesh */
  if ( abs(mesh->info.imprim) > 4 || mesh->info.ddebug )
    fprintf(stdout,"  ** COMPUTATIONAL MESH\n");

  if ( !MMG2D_defsiz(mesh,met) ) {
    fprintf(stdout,"  ## Metric undefined. Exit program.\n");
    return(0);
  }

  if ( mesh->info.hgrad > 0. ) {
    if ( mesh->info.imprim )   fprintf(stdout,"\n  -- GRADATION : %8f\n",mesh->info.hgrad);
    if (!MMG2D_gradsiz(mesh,met) ) {
      fprintf(stdout,"  ## Gradation problem. Exit program.\n");
      return(0);
    }
  }
  if ( !_MMG2_anatri(mesh,met,2) ) {
    fprintf(stdout,"  ## Unable to proceed adaptation. Exit program.\n");
    return(0);
  }
  
  /* Stage 3: fine mesh improvements */
  if ( !_MMG2_adptri(mesh,met) ) {
    fprintf(stdout,"  ## Unable to make fine improvements. Exit program.\n");
    return(0);
  }

  /* To remove !!!!!!!!!!!!! */
  /*{
    printf("Saving mesh...\n");
    if ( !MMG2_hashTria(mesh) ) {
      fprintf(stdout,"  ## Hashing problem. Exit program.\n");
      return(0);
    }

    MMG2_bdryEdge(mesh);
    _MMG2_savemesh_db(mesh,mesh->nameout,0);
    _MMG2_savemet_db(mesh,met,mesh->nameout,0);
    exit(EXIT_FAILURE);
  }*/

  return(1);
}
