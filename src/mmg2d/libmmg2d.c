/* =============================================================================
**  This file is part of the mmg software package for the tetrahedral
**  mesh modification.
**  Copyright (c) Bx INP/CNRS/Inria/UBordeaux/UPMC, 2004-
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
#include "mmg2d.h"
#include "mmg2dexterns.h"

/**
 * Pack the mesh \a mesh and its associated metric \a met and/or solution \a sol
 * and return \a val.
 */
#define MMG2D_RETURN_AND_PACK(mesh,met,sol,val)do                       \
  {                                                                     \
    if ( !MMG2D_pack(mesh,met,sol) ) {                                  \
      mesh->npi = mesh->np;                                             \
      mesh->nti = mesh->nt;                                             \
      mesh->nai = mesh->na;                                             \
      mesh->nei = mesh->ne;                                             \
      mesh->xt  = 0;                                                    \
      if ( met ) { met->npi  = met->np; }                               \
      if ( sol ) { sol->npi  = sol->np; }                               \
      return MMG5_LOWFAILURE;                                           \
  }                                                                     \
    _LIBMMG5_RETURN(mesh,met,sol,val);                                  \
    }while(0)

/**
 * \param mesh pointer toward the mesh structure.
 * \param met pointer toward the solution structure.
 *
 * Truncate the metric computed by the DoSol function by hmax and hmin values
 * (if setted by the user). Set hmin and hmax if they are not setted.
 *
 * \warning works only for a metric computed by the DoSol function because we
 * suppose that we have a diagonal tensor in aniso.
 *
 */
void MMG2D_solTruncatureForOptim(MMG5_pMesh mesh, MMG5_pSol met) {
  MMG5_pTria  ptt;
  MMG5_pPoint ppt;
  int         k,i,iadr;
  double      isqhmin, isqhmax;
  int8_t      sethmin, sethmax;

  assert ( mesh->info.optim || mesh->info.hsiz > 0. );

  /* Detect the point used only by quads */
  if ( mesh->nquad ) {
    for (k=1; k<=mesh->np; k++) {
      mesh->point[k].flag = 1;
    }
    for (k=1; k<=mesh->nt; k++) {
      ptt = &mesh->tria[k];
      if ( !MG_EOK(ptt) ) continue;

      for (i=0; i<3; i++) {
        mesh->point[ptt->v[i]].flag = 0;
      }
    }
  }

  /* If not provided by the user, compute hmin/hmax from the metric computed by
   * the DoSol function. */
  sethmin = sethmax = 1;
  if ( mesh->info.hmin < 0. ) {
    sethmin = 0;
    if ( met->size == 1 ) {
      mesh->info.hmin = FLT_MAX;
      for (k=1; k<=mesh->np; k++)  {
        ppt = &mesh->point[k];
        if ( (!MG_VOK(ppt)) || ppt->flag ) continue;
        mesh->info.hmin = MG_MIN(mesh->info.hmin,met->m[k]);
      }
    }
    else if ( met->size == 3 ){
      mesh->info.hmin = 0.;
      for (k=1; k<=mesh->np; k++)  {
        ppt = &mesh->point[k];
        if ( (!MG_VOK(ppt)) || ppt->flag ) continue;
        iadr = met->size*k;
        mesh->info.hmin = MG_MAX(mesh->info.hmin,met->m[iadr]);
        mesh->info.hmin = MG_MAX(mesh->info.hmin,met->m[iadr+2]);
      }
      mesh->info.hmin = 1./sqrt(mesh->info.hmin);
    }
  }
  if ( mesh->info.hmax < 0. ) {
    sethmax = 1;
    if ( met->size == 1 ) {
      mesh->info.hmax = 0.;
      for (k=1; k<=mesh->np; k++)  {
        ppt = &mesh->point[k];
        if ( !MG_VOK(ppt) ) continue;
        mesh->info.hmax = MG_MAX(mesh->info.hmax,met->m[k]);
      }
    }
    else if ( met->size == 3 ){
      mesh->info.hmax = FLT_MAX;
      for (k=1; k<=mesh->np; k++)  {
        ppt = &mesh->point[k];
        if ( (!MG_VOK(ppt)) || ppt->flag ) continue;
        iadr = met->size*k;
        mesh->info.hmax = MG_MIN(mesh->info.hmax,met->m[iadr]);
        mesh->info.hmax = MG_MIN(mesh->info.hmax,met->m[iadr+2]);
      }
      mesh->info.hmax = 1./sqrt(mesh->info.hmax);
    }
  }

  if ( !sethmin ) {
    mesh->info.hmin *=.1;
    /* Check that user has not given a hmax value lower that the founded
     * hmin. */
    if ( mesh->info.hmin > mesh->info.hmax ) {
      mesh->info.hmin = 0.1*mesh->info.hmax;
    }
  }
  if ( !sethmax ) {
    mesh->info.hmax *=10.;
    /* Check that user has not given a hmin value bigger that the founded
     * hmax. */
    if ( mesh->info.hmax < mesh->info.hmin ) {
      mesh->info.hmax = 10.*mesh->info.hmin;
    }
  }

  /* vertex size */
  if ( met->size == 1 ) {
    for (k=1; k<=mesh->np; k++) {
      met->m[k] = MG_MIN(mesh->info.hmax,MG_MAX(mesh->info.hmin,met->m[k]));
    }
  }
  else if ( met->size==3 ) {
    isqhmin = 1./(mesh->info.hmin*mesh->info.hmin);
    isqhmax = 1./(mesh->info.hmax*mesh->info.hmax);

    for (k=1; k<=mesh->np; k++) {
      iadr = 3*k;
      met->m[iadr]   = MG_MAX(isqhmax,MG_MIN(isqhmin,met->m[iadr]));
      met->m[iadr+2] = met->m[iadr];
    }
  }
  return;
}

int MMG2D_mmg2dlib(MMG5_pMesh mesh,MMG5_pSol met)
{
  MMG5_pSol sol=NULL; // unused
  mytime    ctim[TIMEMAX];
  char      stim[32];

  if ( mesh->info.imprim >= 0 ) {
    fprintf(stdout,"\n  %s\n   MODULE MMG2D: %s (%s)\n  %s\n",
            MG_STR,MMG_VERSION_RELEASE,MMG_RELEASE_DATE,MG_STR);
#ifndef _WIN32
    fprintf(stdout,"     git branch: %s\n",MMG_GIT_BRANCH);
    fprintf(stdout,"     git commit: %s\n",MMG_GIT_COMMIT);
    fprintf(stdout,"     git date:   %s\n\n",MMG_GIT_DATE);
#endif
  }

  assert ( mesh );
  assert ( met );
  assert ( mesh->point );
  assert ( mesh->tria );

  /*uncomment to callback*/
  //MMG2D_callbackinsert = titi;

 /* interrupts */
  signal(SIGABRT,MMG2D_excfun);
  signal(SIGFPE,MMG2D_excfun);
  signal(SIGILL,MMG2D_excfun);
  signal(SIGSEGV,MMG2D_excfun);
  signal(SIGTERM,MMG2D_excfun);
  signal(SIGINT,MMG2D_excfun);

  tminit(ctim,TIMEMAX);
  chrono(ON,&(ctim[0]));

  /* Check options */
  if ( !mesh->nt ) {
    fprintf(stdout,"\n  ## ERROR: NO TRIANGLES IN THE MESH. \n");
    fprintf(stdout,"          To generate a mesh from boundaries call the"
            " MMG2D_mmg2dmesh function\n.");
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }
  else if ( mesh->info.iso ) {
    fprintf(stdout,"\n  ## ERROR: LEVEL-SET DISCRETISATION UNAVAILABLE"
            " (MMG2D_IPARAM_iso):\n"
            "          YOU MUST CALL THE MMG2D_mmg2dls FUNCTION TO USE THIS"
            " OPTION.\n");
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }
  else if ( mesh->info.lag >= 0 ) {
    fprintf(stdout,"\n  ## ERROR: LAGRANGIAN MODE UNAVAILABLE (MMG2D_IPARAM_lag):\n"
            "            YOU MUST CALL THE MMG2D_mmg2dmov FUNCTION TO MOVE A"
            " RIGIDBODY.\n");
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }

  if ( mesh->info.imprim > 0 ) fprintf(stdout,"\n  -- MMG2DLIB: INPUT DATA\n");

  /* Check input */
  chrono(ON,&(ctim[1]));

  if ( met->np && ( met->np != mesh->np ) ) {
    fprintf(stdout,"\n  ## WARNING: WRONG SOLUTION NUMBER : %d != %d\n",met->np,mesh->np);
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }
  else if ( met->size!=1 && met->size!=3 ) {
    fprintf(stderr,"\n  ## ERROR: WRONG DATA TYPE.\n");
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }

  /* specific meshing */
  if ( met->np ) {
    if ( mesh->info.optim ) {
      printf("\n  ## ERROR: MISMATCH OPTIONS: OPTIM OPTION CAN NOT BE USED"
             " WITH AN INPUT METRIC.\n");
      _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    }

    if ( mesh->info.hsiz>0. ) {
      printf("\n  ## ERROR: MISMATCH OPTIONS: HSIZ OPTION CAN NOT BE USED"
             " WITH AN INPUT METRIC.\n");
      _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    }
  }

  if ( mesh->info.optim &&  mesh->info.hsiz>0. ) {
    printf("\n  ## ERROR: MISMATCH OPTIONS: HSIZ AND OPTIM OPTIONS CAN NOT BE USED"
           " TOGETHER.\n");
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }

  chrono(OFF,&(ctim[1]));
  printim(ctim[1].gdif,stim);

  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"  --  INPUT DATA COMPLETED.     %s\n",stim);

  /* Set function pointers */
  MMG2D_setfunc(mesh,met);
  MMG2D_Set_commonFunc();

  if ( abs(mesh->info.imprim) > 5 || mesh->info.ddebug ) {
    fprintf(stdout,"  MAXIMUM NUMBER OF POINTS    (NPMAX) : %8d\n",mesh->npmax);
    fprintf(stdout,"  MAXIMUM NUMBER OF TRIANGLES (NTMAX) : %8d\n",mesh->ntmax);
  }

  /* Data analysis */
  chrono(ON,&ctim[2]);
  if ( mesh->info.imprim > 0 )   fprintf(stdout,"\n  -- PHASE 1 : DATA ANALYSIS\n");

  /* Keep only one domain if asked */
  MMG2D_keep_only1Subdomain ( mesh, mesh->info.nsd );

  /* reset fem value to user setting (needed for multiple library call) */
  mesh->info.fem = mesh->info.setfem;

  /* Scale input mesh */
  if ( !MMG2D_scaleMesh(mesh,met,NULL) )  _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);

  /* Specific meshing */
  if ( mesh->info.optim ) {
    if ( !MMG2D_doSol(mesh,met) ) {
     if ( !MMG5_unscaleMesh(mesh,met,NULL) ) _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
     _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    }
    MMG2D_solTruncatureForOptim(mesh,met);
  }

  if ( mesh->info.hsiz > 0. ) {
    if ( !MMG2D_Set_constantSize(mesh,met) ) {
     if ( !MMG5_unscaleMesh(mesh,met,NULL) ) _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
     _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    }
  }

  /* Print initial quality history */
  if ( mesh->info.imprim > 0  ||  mesh->info.imprim < -1 ) {
    if ( !MMG2D_outqua(mesh,met) ) {
      if ( !MMG5_unscaleMesh(mesh,met,NULL) ) _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
      _LIBMMG5_RETURN(mesh,met,sol,MMG5_LOWFAILURE);
    }
  }

  /* Mesh analysis */
  if (! MMG2D_analys(mesh) ) {
    if ( !MMG5_unscaleMesh(mesh,met,NULL) ) _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_LOWFAILURE);
  }

  if ( mesh->info.ddebug && !MMG5_chkmsh(mesh,1,0) )  _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);

  if ( mesh->info.imprim > 1 && met->m && met->np ) MMG2D_prilen(mesh,met);

  chrono(OFF,&(ctim[2]));
  printim(ctim[2].gdif,stim);
  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"  -- PHASE 1 COMPLETED.     %s\n",stim);

  /* remeshing */
  chrono(ON,&ctim[3]);

  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"\n  -- PHASE 2 : %s MESHING\n",met->size < 3 ? "ISOTROPIC" : "ANISOTROPIC");

  /* Mesh improvement */
  if ( !MMG2D_mmg2d1n(mesh,met) ) {
    if ( !MMG5_unscaleMesh(mesh,met,NULL) )  _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    MMG2D_RETURN_AND_PACK(mesh,met,sol,MMG5_LOWFAILURE);
  }

  chrono(OFF,&(ctim[3]));
  printim(ctim[3].gdif,stim);
  if ( mesh->info.imprim > 0 ) {
    fprintf(stdout,"  -- PHASE 2 COMPLETED.     %s\n",stim);
  }

  /* Print output quality history */
  if ( !MMG2D_outqua(mesh,met) ) {
     if ( !MMG5_unscaleMesh(mesh,met,NULL) )  _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
     MMG2D_RETURN_AND_PACK(mesh,met,sol,MMG5_LOWFAILURE);
   }

  /* Print edge length histories */
  if ( abs(mesh->info.imprim) > 4 )  {
    MMG2D_prilen(mesh,met);
  }

  chrono(ON,&(ctim[1]));
  if ( mesh->info.imprim > 0 )  fprintf(stdout,"\n  -- MESH PACKED UP\n");

  /* Unscale mesh */
  if ( !MMG5_unscaleMesh(mesh,met,NULL) )  _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  if (!MMG2D_pack(mesh,met,sol) ) _LIBMMG5_RETURN(mesh,met,sol,MMG5_LOWFAILURE);

  chrono(OFF,&(ctim[1]));

  chrono(OFF,&ctim[0]);
  printim(ctim[0].gdif,stim);
  if ( mesh->info.imprim >= 0 ) {
    fprintf(stdout,"\n   MMG2DLIB: ELAPSED TIME  %s\n",stim);
    fprintf(stdout,"\n  %s\n   END OF MODULE MMG2D\n  %s\n\n",MG_STR,MG_STR);
  }
  _LIBMMG5_RETURN(mesh,met,sol,MMG5_SUCCESS);

}

/**
 * \param mesh pointer toward the mesh structure.
 * \return 0 if fail (lack of memory), 1 otherwise.
 *
 * Clean the mesh structure when we just call the MMG2D_Free_Triangles and
 * MMG2D_Free_Edges functions between 2 call of the MMG2D_mmg2dmesh function:
 *   - Allocate the tria and edge structures if needed;
 *   - Reset the tags at vertices.
 *
 */
static inline
int MMG2D_restart(MMG5_pMesh mesh){
  int k;

  /** If needed, reallocate the missing structures */
  if ( !mesh->tria ) {
    /* If we call the library more than one time and if we free the triangles
     * using the MMG2D_Free_triangles function we need to reallocate it */
    MMG5_ADD_MEM(mesh,(mesh->ntmax+1)*sizeof(MMG5_Tria),
                  "initial triangles",return 0);
    MMG5_SAFE_CALLOC(mesh->tria,mesh->ntmax+1,MMG5_Tria,return 0);
    mesh->nenil = mesh->nt + 1;
    for ( k=mesh->nenil; k<mesh->ntmax-1; k++) {
      mesh->tria[k].v[2] = k+1;
    }
  }
  if ( mesh->na && !mesh->edge ) {
    /* If we call the library more than one time and if we free the triangles
     * using the MMG2D_Free_triangles function we need to reallocate it */
    MMG5_ADD_MEM(mesh,(mesh->namax+1)*sizeof(MMG5_Edge),
                  "initial edges",return 0);
    MMG5_SAFE_CALLOC(mesh->edge,mesh->namax+1,MMG5_Edge,return 0);
    if ( mesh->na < mesh->namax ) {
      mesh->nanil = mesh->na + 1;
    }
    else
      mesh->nanil = 0;
  }

  return 1;
}


int MMG2D_mmg2dmesh(MMG5_pMesh mesh,MMG5_pSol met) {
  MMG5_pSol sol=NULL; // unused
  mytime    ctim[TIMEMAX];
  char      stim[32];

  if ( mesh->info.imprim >= 0 ) {
    fprintf(stdout,"\n  %s\n   MODULE MMG2D: %s (%s)\n  %s\n",
            MG_STR,MMG_VERSION_RELEASE,MMG_RELEASE_DATE,MG_STR);
#ifndef _WIN32
    fprintf(stdout,"     git branch: %s\n",MMG_GIT_BRANCH);
    fprintf(stdout,"     git commit: %s\n",MMG_GIT_COMMIT);
    fprintf(stdout,"     git date:   %s\n\n",MMG_GIT_DATE);
#endif
  }

  assert ( mesh );
  assert ( met );
  assert ( mesh->point );

  /*uncomment for callback*/
  //MMG2D_callbackinsert = titi;

  /* interrupts */
  signal(SIGABRT,MMG2D_excfun);
  signal(SIGFPE,MMG2D_excfun);
  signal(SIGILL,MMG2D_excfun);
  signal(SIGSEGV,MMG2D_excfun);
  signal(SIGTERM,MMG2D_excfun);
  signal(SIGINT,MMG2D_excfun);

  tminit(ctim,TIMEMAX);
  chrono(ON,&(ctim[0]));

  /* Check options */
  if ( mesh->nt ) {
    fprintf(stdout,"\n  ## ERROR: YOUR MESH CONTAINS ALREADY TRIANGLES.\n"
            " THE MESH GENERATION OPTION IS UNAVAILABLE.\n");
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }

  else if ( mesh->info.iso ) {
    fprintf(stdout,"\n  ## ERROR: LEVEL-SET DISCRETISATION UNAVAILABLE"
            " (MMG2D_IPARAM_iso):\n"
            "          YOU MUST CALL THE MMG2D_MMG2DLS FUNCTION TO USE THIS OPTION.\n");
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }

  else if ( mesh->info.lag >= 0 ) {
    fprintf(stdout,"\n  ## ERROR: LAGRANGIAN MODE UNAVAILABLE (MMG2D_IPARAM_lag):\n"
            "            YOU MUST CALL THE MMG2D_MMG2DMOV FUNCTION TO MOVE A RIGIDBODY.\n");
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }
  /* specific meshing */
  if ( met->np ) {
    if ( mesh->info.optim ) {
      printf("\n  ## ERROR: MISMATCH OPTIONS: OPTIM OPTION CAN NOT BE USED"
             " WITH AN INPUT METRIC.\n");
      _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    }

    if ( mesh->info.hsiz>0. ) {
      printf("\n  ## ERROR: MISMATCH OPTIONS: HSIZ OPTION CAN NOT BE USED"
             " WITH AN INPUT METRIC.\n");
      _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    }
  }

  if ( mesh->info.optim &&  mesh->info.hsiz>0. ) {
    printf("\n  ## ERROR: MISMATCH OPTIONS: HSIZ AND OPTIM OPTIONS CAN NOT BE USED"
           " TOGETHER.\n");
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }

  if ( mesh->info.imprim > 0 ) fprintf(stdout,"\n  -- MMG2DMESH: INPUT DATA\n");

  /* Check input */
  chrono(ON,&(ctim[1]));

  if ( met->np && (met->np != mesh->np) ) {
    fprintf(stdout,"\n  ## WARNING: WRONG SOLUTION NUMBER : %d != %d\n",met->np,mesh->np);
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }  else if ( met->size!=1 && met->size!=3 ) {
    fprintf(stderr,"\n  ## ERROR: WRONG DATA TYPE.\n");
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }

  chrono(OFF,&(ctim[1]));
  printim(ctim[1].gdif,stim);
  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"  --  INPUT DATA COMPLETED.     %s\n",stim);

  /* Create function pointers */
  MMG2D_setfunc(mesh,met);
  MMG2D_Set_commonFunc();

  if ( abs(mesh->info.imprim) > 5 || mesh->info.ddebug ) {
    fprintf(stdout,"  MAXIMUM NUMBER OF POINTS    (NPMAX) : %8d\n",mesh->npmax);
    fprintf(stdout,"  MAXIMUM NUMBER OF TRIANGLES (NTMAX) : %8d\n",mesh->ntmax);
  }

  /* analysis */
  chrono(ON,&ctim[2]);

  if ( !MMG2D_restart(mesh) ) {
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }

  if ( mesh->info.imprim > 0 )   fprintf(stdout,"\n  -- PHASE 1 : MESH GENERATION\n");

  /* reset fem value to user setting (needed for multiple library call) */
  mesh->info.fem = mesh->info.setfem;

  /* scaling mesh */
  if ( !MMG2D_scaleMesh(mesh,met,NULL) )  _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);

  if ( mesh->info.ddebug && !MMG5_chkmsh(mesh,1,0) )  _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);

  /* Memory alloc */
  MMG5_ADD_MEM(mesh,(3*mesh->ntmax+5)*sizeof(int),"adjacency table",
                printf("  Exit program.\n");
                return MMG5_STRONGFAILURE);
  MMG5_SAFE_CALLOC(mesh->adja,3*mesh->ntmax+5,int,return MMG5_STRONGFAILURE);

  /* Delaunay triangulation of the set of points contained in the mesh,
   * enforcing the edges of the mesh */
  if ( !MMG2D_mmg2d2(mesh,met) )  {
    if ( !MMG5_unscaleMesh(mesh,met,NULL) )  _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    MMG2D_RETURN_AND_PACK(mesh,met,sol,MMG5_LOWFAILURE);
  }

  chrono(OFF,&(ctim[2]));
  printim(ctim[2].gdif,stim);
  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"  -- PHASE 1 COMPLETED.     %s\n",stim);

  /* remeshing */
  chrono(ON,&ctim[3]);
  if ( mesh->info.imprim > 0 ) {
    fprintf(stdout,"\n  -- PHASE 2 : ANALYSIS\n");
  }

  /* specific meshing */
  if ( mesh->info.optim ) {
    if ( !MMG2D_doSol(mesh,met) ) {
      if ( !MMG5_unscaleMesh(mesh,met,NULL) )
        _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
      MMG2D_RETURN_AND_PACK(mesh,met,sol,MMG5_LOWFAILURE);
    }
    MMG2D_solTruncatureForOptim(mesh,met);
  } else if (mesh->info.hsiz > 0.) {
    if ( !MMG2D_Set_constantSize(mesh,met) ) {
     if ( !MMG5_unscaleMesh(mesh,met,NULL) ) _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
     _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    }
  } else {
    /* Set default hmin and hmax values */
    if ( !MMG5_Set_defaultTruncatureSizes(mesh,mesh->info.hmin>0.,mesh->info.hmax>0.) ) {
      if ( !MMG5_unscaleMesh(mesh,met,NULL) )
        _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
      MMG2D_RETURN_AND_PACK(mesh,met,sol,MMG5_LOWFAILURE);
    }
  }

  /* Mesh analysis */
  if (! MMG2D_analys(mesh) ) {
    if ( !MMG5_unscaleMesh(mesh,met,NULL) )
      _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    MMG2D_RETURN_AND_PACK(mesh,met,sol,MMG5_LOWFAILURE);
  }

  chrono(OFF,&(ctim[3]));
  printim(ctim[3].gdif,stim);
  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"  -- PHASE 2 COMPLETED.     %s\n",stim);

  /* Mesh improvement - call new version of mmg2d1 */
  chrono(ON,&(ctim[4]));
  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"\n  -- PHASE 3 : MESH IMPROVEMENT (%s)\n",
            met->size < 3 ? "ISOTROPIC" : "ANISOTROPIC");

  if ( !MMG2D_mmg2d1n(mesh,met) ) {
    if ( !MMG5_unscaleMesh(mesh,met,NULL) )  _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    MMG2D_RETURN_AND_PACK(mesh,met,sol,MMG5_LOWFAILURE);
  }

  chrono(OFF,&(ctim[4]));
  printim(ctim[4].gdif,stim);
  if ( mesh->info.imprim > 0 ) {
    fprintf(stdout,"  -- PHASE 3 COMPLETED.     %s\n",stim);
  }

  /* Unscale mesh */
  if ( !MMG5_unscaleMesh(mesh,met,NULL) )  _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);

  /* Print quality histories */
  if ( !MMG2D_outqua(mesh,met) ) {
     MMG2D_RETURN_AND_PACK(mesh,met,sol,MMG5_LOWFAILURE);
  }

  /* Print edge length histories */
  if ( abs(mesh->info.imprim) > 4 )  {
    MMG2D_prilen(mesh,met);
  }

  chrono(ON,&(ctim[1]));
  if ( mesh->info.imprim > 0 )  fprintf(stdout,"\n  -- MESH PACKED UP\n");

  if (!MMG2D_pack(mesh,met,sol) ) _LIBMMG5_RETURN(mesh,met,sol,MMG5_LOWFAILURE);

  chrono(OFF,&(ctim[1]));

  chrono(OFF,&ctim[0]);
  printim(ctim[0].gdif,stim);
  if ( mesh->info.imprim >= 0 ) {
    fprintf(stdout,"\n   MMG2DMESH: ELAPSED TIME  %s\n",stim);
    fprintf(stdout,"\n  %s\n   END OF MODULE MMG2D\n  %s\n\n",MG_STR,MG_STR);
  }

  _LIBMMG5_RETURN(mesh,met,sol,MMG5_SUCCESS);

}

int MMG2D_mmg2dls(MMG5_pMesh mesh,MMG5_pSol sol,MMG5_pSol umet)
{
  MMG5_pSol met=NULL;
  mytime    ctim[TIMEMAX];
  char      stim[32];
  int8_t    mettofree = 0;

  if ( mesh->info.imprim >= 0 ) {
    fprintf(stdout,"\n  %s\n   MODULE MMG2D: %s (%s)\n  %s\n",
            MG_STR,MMG_VERSION_RELEASE,MMG_RELEASE_DATE,MG_STR);
#ifndef _WIN32
    fprintf(stdout,"     git branch: %s\n",MMG_GIT_BRANCH);
    fprintf(stdout,"     git commit: %s\n",MMG_GIT_COMMIT);
    fprintf(stdout,"     git date:   %s\n\n",MMG_GIT_DATE);
#endif
  }

  assert ( mesh );
  assert ( sol );
  assert ( mesh->point );
  assert ( mesh->tria );

  if ( !mesh->info.iso ) { mesh->info.iso = 1; }

  if ( !umet ) {
    /* User doesn't provide the metric, allocate our own one */
    MMG5_SAFE_CALLOC(met,1,MMG5_Sol,_LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE));
    mettofree = 1;
  }
  else {
    met = umet;
  }

  /*uncomment for callback*/
  //MMG2D_callbackinsert = titi;

  /* interrupts */
  signal(SIGABRT,MMG2D_excfun);
  signal(SIGFPE,MMG2D_excfun);
  signal(SIGILL,MMG2D_excfun);
  signal(SIGSEGV,MMG2D_excfun);
  signal(SIGTERM,MMG2D_excfun);
  signal(SIGINT,MMG2D_excfun);

  tminit(ctim,TIMEMAX);
  chrono(ON,&(ctim[0]));

  /* Check options */
  if ( mesh->info.lag >= 0 ) {
    fprintf(stdout,"\n  ## ERROR: LAGRANGIAN MODE UNAVAILABLE (MMG2D_IPARAM_lag):\n"
            "            YOU MUST CALL THE MMG2D_mmg2dmov FUNCTION TO MOVE A RIGIDBODY.\n");
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
  }

  /* Check input */
  if ( mesh->info.imprim > 0 ) fprintf(stdout,"\n  -- MMG2DLS: INPUT DATA\n");
  chrono(ON,&(ctim[1]));

  sol->ver  = mesh->ver;

  if ( !mesh->nt ) {
    fprintf(stdout,"\n  ## ERROR: NO TRIANGLES IN THE MESH \n");
    if ( mettofree ) { MMG5_SAFE_FREE (met); }
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
  }
  else if ( !sol->m ) {
    fprintf(stdout,"\n  ## ERROR: A VALID SOLUTION FILE IS NEEDED \n");
    if ( mettofree ) { MMG5_SAFE_FREE (met); }
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
  } else   if ( sol->size != 1 ) {
    fprintf(stdout,"\n  ## ERROR: WRONG DATA TYPE.\n");
    if ( mettofree ) { MMG5_SAFE_FREE (met); }
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
  } else if ( sol->np && (sol->np != mesh->np) ) {
    fprintf(stdout,"\n  ## WARNING: WRONG SOLUTION NUMBER. IGNORED\n");
    if ( mettofree ) { MMG5_SAFE_FREE (met); }
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
  }

  /* specific meshing */
  if ( met && met->np ) {
  if ( mesh->info.optim ) {
      printf("\n  ## ERROR: MISMATCH OPTIONS: OPTIM OPTION CAN NOT BE USED"
             " WITH AN INPUT METRIC.\n");
      if ( mettofree ) { MMG5_SAFE_FREE (met); }
      _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }

  if ( mesh->info.hsiz>0. ) {
      printf("\n  ## ERROR: MISMATCH OPTIONS: HSIZ OPTION CAN NOT BE USED"
             " WITH AN INPUT METRIC.\n");
      if ( mettofree ) { MMG5_SAFE_FREE (met); }
      _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    }
    if ( met->np != mesh->np ) {
      fprintf(stdout,"\n  ## WARNING: WRONG METRIC NUMBER. IGNORED\n");
      if ( mettofree ) { MMG5_SAFE_FREE (met); }
      _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
    }
  }

  if ( mesh->info.optim &&  mesh->info.hsiz>0. ) {
    printf("\n  ## ERROR: MISMATCH OPTIONS: HSIZ AND OPTIM OPTIONS CAN NOT BE USED"
           " TOGETHER.\n");
    if ( mettofree ) { MMG5_SAFE_FREE (met); }
    _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
  }

  if ( mesh->nquad && mesh->quadra ) {
    printf("\n  ## ERROR: UNABLE TO HANDLE HYBRID MESHES IN ISOVALUE DISCRETIZATION MODE.\n");
    if ( mettofree ) { MMG5_SAFE_FREE (met); }
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
  }

  chrono(OFF,&(ctim[1]));
  printim(ctim[1].gdif,stim);
  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"  --  INPUT DATA COMPLETED.     %s\n",stim);

  chrono(ON,&ctim[2]);

  /* Set pointers */
  MMG2D_setfunc(mesh,met);
  MMG2D_Set_commonFunc();

  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"\n  -- PHASE 1 : ISOSURFACE DISCRETIZATION\n");

  if ( abs(mesh->info.imprim) > 5 || mesh->info.ddebug ) {
    fprintf(stdout,"  MAXIMUM NUMBER OF POINTS    (NPMAX) : %8d\n",mesh->npmax);
    fprintf(stdout,"  MAXIMUM NUMBER OF TRIANGLES (NTMAX) : %8d\n",mesh->ntmax);
  }

  /* reset fem value to user setting (needed for multiple library call) */
  mesh->info.fem = mesh->info.setfem;

  /* scaling mesh */
  if ( !MMG2D_scaleMesh(mesh,met,sol) ) {
    if ( mettofree ) { MMG5_SAFE_FREE (met); }
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
  }

  if ( mesh->nt && !MMG2D_hashTria(mesh) ) {
    if ( mettofree ) { MMG5_SAFE_FREE (met); }
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
  }

  if ( mesh->info.ddebug && !MMG5_chkmsh(mesh,1,0) ) {
    if ( mettofree ) { MMG5_SAFE_FREE (met); }
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
  }

  /* Print initial quality */
  if ( mesh->info.imprim > 0  ||  mesh->info.imprim < -1 ) {
    if ( !MMG2D_outqua(mesh,met) ) {
    if ( mettofree ) {
      MMG5_DEL_MEM(mesh,met->m);
      MMG5_SAFE_FREE (met);
    }
      if ( !MMG5_unscaleMesh(mesh,met,sol) ) {
        _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
      }
      MMG2D_RETURN_AND_PACK(mesh,sol,met,MMG5_LOWFAILURE);
    }
  }

  /* Discretization of the mesh->info.ls isovalue of sol in the mesh */
  if ( !MMG2D_mmg2d6(mesh,sol,umet) ) {
    if ( mettofree ) { MMG5_SAFE_FREE (met); }
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
  }

  chrono(OFF,&(ctim[2]));
  printim(ctim[2].gdif,stim);
  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"  -- PHASE 1 COMPLETED.     %s\n",stim);

  chrono(ON,&(ctim[3]));
  if ( mesh->info.imprim > 0 ) {
    fprintf(stdout,"\n  -- PHASE 2 : ANALYSIS\n");
  }

  /* specific meshing */
  if ( mesh->info.optim ) {
    if ( !MMG2D_doSol(mesh,met) ) {
      if ( mettofree ) {
        MMG5_DEL_MEM(mesh,met->m);
        MMG5_SAFE_FREE (met);
      }

      if ( !MMG5_unscaleMesh(mesh,met,NULL) ) {
        _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE); }
      MMG2D_RETURN_AND_PACK(mesh,met,sol,MMG5_LOWFAILURE);
    }
    MMG2D_solTruncatureForOptim(mesh,met);
  }
  if ( mesh->info.hsiz > 0. ) {
    if ( !MMG2D_Set_constantSize(mesh,met) ) {
      if ( mettofree ) {
        MMG5_DEL_MEM(mesh,met->m);
        MMG5_SAFE_FREE (met);
      }
      if ( !MMG5_unscaleMesh(mesh,met,sol) ) {
        _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
      }
     _LIBMMG5_RETURN(mesh,met,sol,MMG5_STRONGFAILURE);
    }
  }

  /* Mesh analysis */
  if (! MMG2D_analys(mesh) ) {
    if ( mettofree ) {
      MMG5_DEL_MEM(mesh,met->m);
      MMG5_SAFE_FREE (met);
    }
    if ( !MMG5_unscaleMesh(mesh,met,NULL) ) {
      _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
    }
    MMG2D_RETURN_AND_PACK(mesh,sol,met,MMG5_LOWFAILURE);
  }

  chrono(OFF,&(ctim[3]));
  printim(ctim[3].gdif,stim);
  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"  -- PHASE 2 COMPLETED.     %s\n",stim);

  /* Mesh improvement - call new version of mmg2d1 */
  chrono(ON,&ctim[4]);
  if ( mesh->info.imprim > 0 ) {
    fprintf(stdout,"\n  -- PHASE 3 : MESH IMPROVEMENT\n");
  }

  if ( !MMG2D_mmg2d1n(mesh,met) ) {
    if ( mettofree ) {
      MMG5_DEL_MEM(mesh,met->m);
      MMG5_SAFE_FREE (met);
    }
    if ( !MMG5_unscaleMesh(mesh,met,NULL) ) _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
    MMG2D_RETURN_AND_PACK(mesh,sol,met,MMG5_LOWFAILURE);
  }

  /* End of mmg2dls */
  chrono(OFF,&(ctim[4]));
  printim(ctim[4].gdif,stim);
  if ( mesh->info.imprim > 0 ) {
    fprintf(stdout,"  -- PHASE 3 COMPLETED.     %s\n",stim);
  }

  /* Unscale mesh */
  if ( !MMG5_unscaleMesh(mesh,met,NULL) ) {
    if ( mettofree ) {
      MMG5_DEL_MEM(mesh,met->m);
      MMG5_SAFE_FREE (met);
    }
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_STRONGFAILURE);
  }

  /* Print quality histories */
  if ( !MMG2D_outqua(mesh,met) ) {
    if ( mettofree ) {
      MMG5_DEL_MEM(mesh,met->m);
      MMG5_SAFE_FREE (met);
    }
    MMG2D_RETURN_AND_PACK(mesh,sol,met,MMG5_LOWFAILURE);
  }

  chrono(ON,&(ctim[1]));
  if ( mesh->info.imprim > 0 )  fprintf(stdout,"\n  -- MESH PACKED UP\n");

  /* Pack mesh */
  if (!MMG2D_pack(mesh,sol,met) ) {
    if ( mettofree ) { MMG5_SAFE_FREE (met); }
    _LIBMMG5_RETURN(mesh,sol,met,MMG5_LOWFAILURE);
  }

  chrono(OFF,&(ctim[1]));

  chrono(OFF,&ctim[0]);
  printim(ctim[0].gdif,stim);
  if ( mesh->info.imprim >= 0 ) {
    fprintf(stdout,"\n   MMG2DLS: ELAPSED TIME  %s\n",stim);
    fprintf(stdout,"\n  %s\n   END OF MODULE MMG2D\n  %s\n\n",MG_STR,MG_STR);
  }
  if ( mettofree ) {
    MMG5_DEL_MEM(mesh,met->m);
    MMG5_SAFE_FREE (met);
  }
  _LIBMMG5_RETURN(mesh,sol,met,MMG5_SUCCESS);

}

int MMG2D_mmg2dmov(MMG5_pMesh mesh,MMG5_pSol met,MMG5_pSol disp) {
  mytime    ctim[TIMEMAX];
  char      stim[32];
  int       ier;
  int       k,*invalidTris;


  if ( mesh->info.imprim >= 0 ) {
    fprintf(stdout,"\n  %s\n   MODULE MMG2D : %s (%s)\n  %s\n",
            MG_STR,MMG_VERSION_RELEASE,MMG_RELEASE_DATE,MG_STR);
#ifndef _WIN32
    fprintf(stdout,"     git branch: %s\n",MMG_GIT_BRANCH);
    fprintf(stdout,"     git commit: %s\n",MMG_GIT_COMMIT);
    fprintf(stdout,"     git date:   %s\n\n",MMG_GIT_DATE);
#endif
  }

  assert ( mesh );
  assert ( disp );
  assert ( met );
  assert ( mesh->point );
  assert ( mesh->tria );

  /*uncomment for callback*/
  //MMG2D_callbackinsert = titi;

  /* interrupts */
  signal(SIGABRT,MMG2D_excfun);
  signal(SIGFPE,MMG2D_excfun);
  signal(SIGILL,MMG2D_excfun);
  signal(SIGSEGV,MMG2D_excfun);
  signal(SIGTERM,MMG2D_excfun);
  signal(SIGINT,MMG2D_excfun);

  tminit(ctim,TIMEMAX);
  chrono(ON,&(ctim[0]));

  /* Check data compatibility */
  if ( mesh->info.imprim > 0 ) fprintf(stdout,"\n  -- MMG2DMOV: INPUT DATA\n");
  chrono(ON,&(ctim[1]));

  disp->ver  = mesh->ver;

#ifndef USE_ELAS
  fprintf(stderr,"\n  ## ERROR: YOU NEED TO COMPILE WITH THE USE_ELAS"
    " CMake's FLAG SET TO ON TO USE THE RIGIDBODY MOVEMENT LIBRARY.\n");
    _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);
#endif

  if ( !mesh->nt ) {
    fprintf(stdout,"\n  ## ERROR: NO TRIANGLES IN THE MESH \n");
    _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);
  }
  else if ( !disp->m ) {
    fprintf(stdout,"\n  ## ERROR: A VALID SOLUTION FILE IS NEEDED \n");
    _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);
  }
  else if ( disp->size != 2 ) {
    fprintf(stdout,"\n  ## ERROR: LAGRANGIAN MOTION OPTION NEED A VECTOR DISPLACEMENT.\n");
    _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);
  }
  else if ( disp->np && (disp->np != mesh->np) ) {
    fprintf(stdout,"\n  ## WARNING: WRONG SOLUTION NUMBER. IGNORED\n");
    MMG5_DEL_MEM(mesh,disp->m);
    disp->np = 0;
  }

  if ( mesh->info.optim ) {
    printf("\n  ## ERROR: OPTIM OPTION UNAVAILABLE IN LAGRANGIAN"
           " MOVEMENT MODE.\n");
    _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);
  }
  if ( mesh->info.hsiz>0. ) {
    printf("\n  ## ERROR: HSIZ OPTION UNAVAILABLE IN LAGRANGIAN"
           " MOVEMENT MODE.\n");
    _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);
  }

  if ( mesh->nquad && mesh->quadra ) {
    printf("\n  ## ERROR: UNABLE TO HANDLE HYBRID MESHES IN LAGRANGIAN MOVEMENT MODE.\n");
    _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);
  }


  chrono(OFF,&(ctim[1]));
  printim(ctim[1].gdif,stim);
  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"  --  INPUT DATA COMPLETED.     %s\n",stim);

  /* Set pointers */
  MMG2D_setfunc(mesh,met);
  MMG2D_Set_commonFunc();

  chrono(ON,&ctim[2]);

  if ( mesh->info.imprim > 0 ) {
    fprintf(stdout,"\n  -- PHASE 1 : ANALYSIS\n");
  }

  if ( abs(mesh->info.imprim) > 5 || mesh->info.ddebug ) {
    fprintf(stdout,"  MAXIMUM NUMBER OF POINTS    (NPMAX) : %8d\n",mesh->npmax);
    fprintf(stdout,"  MAXIMUM NUMBER OF TRIANGLES (NTMAX) : %8d\n",mesh->ntmax);
  }

  /* Analysis */

  /* reset fem value to user setting (needed for multiple library call) */
  mesh->info.fem = mesh->info.setfem;

  /* scaling mesh  */
  if ( !MMG2D_scaleMesh(mesh,NULL,disp) )  _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);

  if ( mesh->nt && !MMG2D_hashTria(mesh) )  _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);
  if ( mesh->info.ddebug && !MMG5_chkmsh(mesh,1,0) )  _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);

  /* Print initial quality */
  if ( mesh->info.imprim > 0  ||  mesh->info.imprim < -1 ) {
    if ( !MMG2D_outqua(mesh,met) ) {
      if ( !MMG5_unscaleMesh(mesh,NULL,disp) )  _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);
      MMG2D_RETURN_AND_PACK(mesh,met,disp,MMG5_LOWFAILURE);
    }
  }

  /* Mesh analysis */
  if (! MMG2D_analys(mesh) )
    _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);

  chrono(OFF,&(ctim[2]));
  printim(ctim[2].gdif,stim);
  if ( mesh->info.imprim > 0 )
    fprintf(stdout,"  -- PHASE 1 COMPLETED.     %s\n",stim);

  /* Lagrangian motion */
  chrono(ON,&(ctim[3]));
  if ( mesh->info.imprim > 0 ) {
    fprintf(stdout,"\n  -- PHASE 2 : LAGRANGIAN MOTION\n");
  }

  /* Lagrangian mode */
  invalidTris = NULL;
  ier = MMG2D_mmg2d9(mesh,disp,met,&invalidTris);
  if ( !ier ) {
    disp->npi = disp->np;
    _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);
  }
  else if ( ier < 0 ) {
    printf("\n  ## Warning: Unable to perform any movement "
           "(%d intersecting triangles).\n",-ier);
    if ( mesh->info.imprim > 1 ) {
      printf("     List of invalid trias: ");
      for ( k=0; k<-ier; ++k ) {
        printf("%d ",MMG2D_indElt(mesh,invalidTris[k]));
      }
      printf("\n\n");
    }
    MMG5_SAFE_FREE(invalidTris);
  }

  chrono(OFF,&(ctim[3]));
  printim(ctim[3].gdif,stim);
  if ( mesh->info.imprim > 0 ) {
    fprintf(stdout,"  -- PHASE 2 COMPLETED.     %s\n",stim);
  }

  /* End with a classical remeshing stage, provided mesh->info.lag > 1 */
  if ( (ier > 0) && (mesh->info.lag >= 1) ) {
    chrono(ON,&(ctim[4]));
    if ( mesh->info.imprim > 0 ) {
      fprintf(stdout,"\n  -- PHASE 3 : MESH IMPROVEMENT\n");
    }

    if ( !MMG2D_mmg2d1n(mesh,met) ) {
      if ( !MMG5_unscaleMesh(mesh,met,NULL) )  _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);
      MMG2D_RETURN_AND_PACK(mesh,met,disp,MMG5_LOWFAILURE);
    }

    chrono(OFF,&(ctim[4]));
    printim(ctim[4].gdif,stim);
    if ( mesh->info.imprim > 0 ) {
      fprintf(stdout,"  -- PHASE 3 COMPLETED.     %s\n",stim);
    }
  }

  /* Unscale mesh */
  if ( !MMG5_unscaleMesh(mesh,met,NULL) )  _LIBMMG5_RETURN(mesh,met,disp,MMG5_STRONGFAILURE);

  /* Print quality histories */
  if ( !MMG2D_outqua(mesh,met) ) {
    MMG2D_RETURN_AND_PACK(mesh,met,disp,MMG5_LOWFAILURE);
  }

  chrono(ON,&(ctim[1]));
  if ( mesh->info.imprim > 0 )  fprintf(stdout,"\n  -- MESH PACKED UP\n");

  /* Pack mesh */
  if (!MMG2D_pack(mesh,met,disp) ) _LIBMMG5_RETURN(mesh,met,disp,MMG5_LOWFAILURE);

  chrono(OFF,&(ctim[1]));

  chrono(OFF,&ctim[0]);
  printim(ctim[0].gdif,stim);
  if ( mesh->info.imprim >= 0 ) {
    fprintf(stdout,"\n   MMG2DMOV: ELAPSED TIME  %s\n",stim);
    fprintf(stdout,"\n  %s\n   END OF MODULE MMG2D\n  %s\n\n",MG_STR,MG_STR);
  }

  _LIBMMG5_RETURN(mesh,met,disp,MMG5_SUCCESS);
}
