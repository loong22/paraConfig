/*
 * $HEADER$
 */

#ifndef OMPI_MPI_EXT_H
#define OMPI_MPI_EXT_H 1

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

#define OMPI_HAVE_MPI_EXT 1

/* Enabled Extension: affinity */
#define OMPI_HAVE_MPI_EXT_AFFINITY 1
#include "openmpi/mpiext/mpiext_affinity_c.h"

/* Enabled Extension: cuda */
#define OMPI_HAVE_MPI_EXT_CUDA 1
#include "openmpi/mpiext/mpiext_cuda_c.h"

/* Enabled Extension: ftmpi */
#define OMPI_HAVE_MPI_EXT_FTMPI 1
#include "openmpi/mpiext/mpiext_ftmpi_c.h"

/* Enabled Extension: rocm */
#define OMPI_HAVE_MPI_EXT_ROCM 1
#include "openmpi/mpiext/mpiext_rocm_c.h"

/* Enabled Extension: shortfloat */
#define OMPI_HAVE_MPI_EXT_SHORTFLOAT 1
#include "openmpi/mpiext/mpiext_shortfloat_c.h"


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* OMPI_MPI_EXT_H */

