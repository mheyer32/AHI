#ifndef DATATYPES_ANIMATIONCLASS_LCS_H
#define DATATYPES_ANIMATIONCLASS_LCS_H

#ifndef DATATYPES_ANIMATIONCLASS_H
#include <datatypes/animationclass.h>
#endif

#ifndef DATATYPES_PICTURECLASS_H
#include <datatypes/pictureclass.h>
#endif


/*****************************************************************************/

/* Animation attributes */

/* The number of animations/video streams stored in the file */
#define ADTA_GetNumAnimations (PDTA_GetNumPictures)

/* The index number of the animation/video stream to load */
#define ADTA_WhichAnimation (PDTA_WhichPicture)


/*****************************************************************************/


/* When querying the number of sounds stored in a file, the following
 * value denotes "the number of sounds is unknown".
 */

#define ADTANUMANIMATIONS_Unknown (0)


#endif /* DATATYPES_ANIMATIONCLASS_LCS_H */
