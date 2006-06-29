#include "copyright.h"
/*==============================================================================
 * FILE: restart.c
 *
 * PURPOSE: Functions for writing and reading restart files.  Restart files
 *   begin with the entire input parameter (athinput.XX) file used to start the
 *   run in text format, with nstep, time, and dt appended, followed by the Gas
 *   array and face-centered B in binary, followed by any problem-specific data
 *   written by a user function in the problem generator.  The restart input
 *   file is parsed by main() using the standard par_* functions, with
 *   the parameters used by init_grid_block() to initialize the grid; values
 *   can be superceded by input from the command line, or another input file.
 *
 * CONTAINS PUBLIC FUNCTIONS: 
 *   restart_grid_block() - reads nstep,time,dt,Gas and B from restart file 
 *   dump_restart()       - writes a restart file
 *
 * VARIABLE TYPE AND STRUCTURE DEFINITIONS: none
 *============================================================================*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "athena.h"
#include "prototypes.h"

/* Assumed maximum line length of lines in parameter file. */
/* Must be same as value in par.c */
#define MAXLEN 256

/*----------------------------------------------------------------------------*/
/* restart_grid_block: Reads nstep,time,dt,arrays of Gas and face-centered B in 
 *   Grid structure from restart file.  The remaining data is initialized
 *   in init_grid_block using input parameters at start of restart file,
 *   the command line, or from a new input file.
 */

void restart_grid_block(char *res_file, Grid *pG)
{
  FILE *fp;
  char line[MAXLEN];
  int i, is = pG->is, ie = pG->ie;
  int j, js = pG->js, je = pG->je;
  int k, ks = pG->ks, ke = pG->ke;
#ifdef MHD
  int ib, jb, kb;
#endif

/* Open the restart file */
  if((fp = fopen(res_file,"r")) == NULL)
    ath_error("[restart_grid_block]: Error opening the restart file\n");

/* Skip over the parameter file at the start of the restart file */
  do{
    fgets(line,MAXLEN,fp);
  }while(strncmp(line,"<par_end>",9) != 0);

/* read nstep */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"N_STEP",6) != 0)
    ath_error("[restart_grid_block]: Expected N_STEP, found %s",line);
  fread(&(pG->nstep),sizeof(int),1,fp);

/* read time */
  fgets(line,MAXLEN,fp);   /* Read the '\n' preceeding the next string */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"TIME",4) != 0)
    ath_error("[restart_grid_block]: Expected TIME, found %s",line);
  fread(&(pG->time),sizeof(Real),1,fp);

/* read dt */
  fgets(line,MAXLEN,fp);    /* Read the '\n' preceeding the next string */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"TIME_STEP",9) != 0)
    ath_error("[restart_grid_block]: Expected TIME_STEP, found %s",line);
  fread(&(pG->dt),sizeof(Real),1,fp);

/* Read the density */
  fgets(line,MAXLEN,fp);    /* Read the '\n' preceeding the next string */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"DENSITY",7) != 0)
    ath_error("[restart_grid_block]: Expected DENSITY, found %s",line);
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fread(&(pG->U[k][j][i].d),sizeof(Real),1,fp);
      }
    }
  }

/* Read the x1-momentum */
  fgets(line,MAXLEN,fp);    /* Read the '\n' preceeding the next string */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"1-MOMENTUM",10) != 0)
    ath_error("[restart_grid_block]: Expected 1-MOMENTUM, found %s",line);
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fread(&(pG->U[k][j][i].M1),sizeof(Real),1,fp);
      }
    }
  }

/* Read the x2-momentum */
  fgets(line,MAXLEN,fp);    /* Read the '\n' preceeding the next string */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"2-MOMENTUM",10) != 0)
    ath_error("[restart_grid_block]: Expected 2-MOMENTUM, found %s",line);
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fread(&(pG->U[k][j][i].M2),sizeof(Real),1,fp);
      }
    }
  }

/* Read the x3-momentum */
  fgets(line,MAXLEN,fp);    /* Read the '\n' preceeding the next string */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"3-MOMENTUM",10) != 0)
    ath_error("[restart_grid_block]: Expected 3-MOMENTUM, found %s",line);
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fread(&(pG->U[k][j][i].M3),sizeof(Real),1,fp);
      }
    }
  }

#ifndef ISOTHERMAL
/* Read energy density */
  fgets(line,MAXLEN,fp);    /* Read the '\n' preceeding the next string */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"ENERGY",6) != 0)
    ath_error("[restart_grid_block]: Expected ENERGY, found %s",line);
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fread(&(pG->U[k][j][i].E),sizeof(Real),1,fp);
      }
    }
  }
#endif

#ifdef MHD
/* if there is more than one cell in each dimension, need to read one more
 * face-centered field component than the number of cells.  [ijbk]b is
 * the number of extra cells to be read  */

  ib = ie > is ? 1 : 0;
  jb = je > js ? 1 : 0;
  kb = ke > ks ? 1 : 0;

/* Read the x1-field */
  fgets(line,MAXLEN,fp);    /* Read the '\n' preceeding the next string */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"1-FIELD",7) != 0)
    ath_error("[restart_grid_block]: Expected 1-FIELD, found %s",line);
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie+ib; i++) {
	fread(&(pG->B1i[k][j][i]),sizeof(Real),1,fp);
      }
    }
  }

/* Read the x2-field */
  fgets(line,MAXLEN,fp);    /* Read the '\n' preceeding the next string */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"2-FIELD",7) != 0)
    ath_error("[restart_grid_block]: Expected 2-FIELD, found %s",line);
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je+jb; j++) {
      for (i=is; i<=ie; i++) {
	fread(&(pG->B2i[k][j][i]),sizeof(Real),1,fp);
      }
    }
  }

/* Read the x3-field */
  fgets(line,MAXLEN,fp);    /* Read the '\n' preceeding the next string */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"3-FIELD",7) != 0)
    ath_error("[restart_grid_block]: Expected 3-FIELD, found %s",line);
  for (k=ks; k<=ke+kb; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fread(&(pG->B3i[k][j][i]),sizeof(Real),1,fp);
      }
    }
  }

/* initialize the cell center magnetic fields as either the average of the face
 * centered field if there is more than one cell in that dimension, or just
 * the face centered field if not  */

  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	pG->U[k][j][i].B1c = 
	  ib ? 0.5*(pG->B1i[k][j][i] + pG->B1i[k][j][i+1]) : pG->B1i[k][j][i];

	pG->U[k][j][i].B2c = 
	  jb ? 0.5*(pG->B2i[k][j][i] + pG->B2i[k][j+1][i]) : pG->B2i[k][j][i];

	pG->U[k][j][i].B3c = 
	  kb ? 0.5*(pG->B3i[k][j][i] + pG->B3i[k+1][j][i]) : pG->B3i[k][j][i];
      }
    }
  }
#endif

  fgets(line,MAXLEN,fp);    /* Read the '\n' preceeding the next string */
  fgets(line,MAXLEN,fp);
  if(strncmp(line,"USER_DATA",9) != 0)
    ath_error("[restart_grid_block]: Expected USER_DATA, found %s",line);

/* Call a user function to read his/her problem-specific data! */
  problem_read_restart(pG, fp);

  fclose(fp);

  return;
}

/*----------------------------------------------------------------------------*/
/* dump_restart: writes a restart file, including problem-specific data from
 *   a user defined function  */

void dump_restart(Grid *pG, Output *pout)
{
  FILE *fp;
  int i, is = pG->is, ie = pG->ie;
  int j, js = pG->js, je = pG->je;
  int k, ks = pG->ks, ke = pG->ke;
#ifdef MHD
  int ib, jb, kb;
#endif

/* Open the output file */
  fp = ath_fopen(pG->outfilename,num_digit,pout->num,NULL,"rst","wb");
  if(fp == NULL){
    fprintf(stderr,"[dump_restart]: Error opening the restart file\n");
    return;
  }

/* Add the current time & nstep to the parameter file */
  par_setd("time","time","%e",pG->time,"Current Simulation Time");
  par_seti("time","nstep","%d",pG->nstep,"Current Simulation Time Step");

/* Write the current state of the parameter file */
  par_dump(2,fp);

/* Write out the current simulation step number */
  fprintf(fp,"N_STEP\n");
  if(fwrite(&(pG->nstep),sizeof(int),1,fp) != 1)
    ath_error("[dump_restart]: fwrite() error\n");

/* Write out the current simulation time */
  fprintf(fp,"\nTIME\n");
  if(fwrite(&(pG->time),sizeof(Real),1,fp) != 1)
    ath_error("[dump_restart]: fwrite() error\n");

/* Write out the current simulation time step */
  fprintf(fp,"\nTIME_STEP\n");
  if(fwrite(&(pG->dt),sizeof(Real),1,fp) != 1)
    ath_error("[dump_restart]: fwrite() error\n");

/* Write the density */
  fprintf(fp,"\nDENSITY\n");
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fwrite(&(pG->U[k][j][i].d),sizeof(Real),1,fp);
      }
    }
  }

/* Write the x1-momentum */
  fprintf(fp,"\n1-MOMENTUM\n");
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fwrite(&(pG->U[k][j][i].M1),sizeof(Real),1,fp);
      }
    }
  }

/* Write the x2-momentum */
  fprintf(fp,"\n2-MOMENTUM\n");
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fwrite(&(pG->U[k][j][i].M2),sizeof(Real),1,fp);
      }
    }
  }

/* Write the x3-momentum */
  fprintf(fp,"\n3-MOMENTUM\n");
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fwrite(&(pG->U[k][j][i].M3),sizeof(Real),1,fp);
      }
    }
  }

#ifndef ISOTHERMAL
/* Write energy density */
  fprintf(fp,"\nENERGY\n");
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fwrite(&(pG->U[k][j][i].E),sizeof(Real),1,fp);
      }
    }
  }
#endif

#ifdef MHD
/* see comments in restart_grid_block() for use of [ijk]b */
  ib = ie > is ? 1 : 0;
  jb = je > js ? 1 : 0;
  kb = ke > ks ? 1 : 0;

/* Write the x1-field */
  fprintf(fp,"\n1-FIELD\n");
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie+ib; i++) {
	fwrite(&(pG->B1i[k][j][i]),sizeof(Real),1,fp);
      }
    }
  }

/* Write the x2-field */
  fprintf(fp,"\n2-FIELD\n");
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je+jb; j++) {
      for (i=is; i<=ie; i++) {
	fwrite(&(pG->B2i[k][j][i]),sizeof(Real),1,fp);
      }
    }
  }

/* Write the x3-field */
  fprintf(fp,"\n3-FIELD\n");
  for (k=ks; k<=ke+kb; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
	fwrite(&(pG->B3i[k][j][i]),sizeof(Real),1,fp);
      }
    }
  }
#endif

  fprintf(fp,"\nUSER_DATA\n");

/* call a user function to write his/her problem-specific data! */
  problem_write_restart(pG, fp);

  fclose(fp);

  return;
}