/****
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWVPARM.H -- LightWave Time-Variant, Textured Parameters
 ****
 */
 
#ifndef LWSDK_VPARM_H
#define LWSDK_VPARM_H

#include <lwtypes.h>
#include <lwio.h>
#include <lwenvel.h>
#include <lwtxtr.h>

#define LWVPARMFUNCS_GLOBAL  "LWVParmFuncs 2"

typedef void *LWVParmID;

/* LWVParm envelope types */
#define LWVP_FLOAT        0x10
#define LWVP_PERCENT      0x20
#define LWVP_DIST         0x30
#define LWVP_ANGLE        0x40
#define LWVP_COLOR        0x51
#define LWVPF_VECTOR      0x01

/* Texture data types */
#define LWVPDT_NOTXTR      -1
#define LWVPDT_VECTOR       0
#define LWVPDT_COLOR        1
#define LWVPDT_PERCENT      2
#define LWVPDT_SCALAR       3
#define LWVPDT_DISPLACEMENT 4

/* state flags */
#define LWVPSF_ENV   (1<<0) // create (set) or has (get) envelope
#define LWVPSF_TEX   (1<<1) // create (set) or has (get) texture

/****
 * VParm Event Notification
 * Texture Event Codes:
 *   TXTRACK    - Generated as texture changes         (eventData == NULL)
 *   TXUPDATE   - Generated after texture has changed  (eventData == NULL)
 *   TXAUTOSIZE - Request for texture autosize         (eventData == bbox[3][2])
 * Envelope Event Codes:
 *   ENVTRACK   - Generated as envelope changes        (eventData == NULL)
 *   ENVUPDATE  - Generated after envelope has changed (eventData == NULL)
 * Envelope and Texture Instance Events:
 *   ENVNEW     - A texture has been created           (eventData == NULL)
 *   ENVOLD     - The texture is being destroyed       (eventData == NULL)
 *   TEXNEW     - An envelope has been created         (eventData == NULL)
 *   TEXOLD     - The envelope is being destroyed      (eventData == NULL)
 ****
 */
typedef enum en_LWVP_EVENTCODES {
  LWVPEC_TXTRACK = 0,
  LWVPEC_TXUPDATE,
  LWVPEC_TXAUTOSIZE,
  LWVPEC_ENVTRACK,
  LWVPEC_ENVUPDATE,
  LWVPEC_ENVNEW,
  LWVPEC_ENVOLD,
  LWVPEC_TEXNEW,
  LWVPEC_TEXOLD,
} en_lwvpec;

typedef int LWVP_EventFunc  ( LWVParmID vp, void *userData,
                              en_lwvpec eventCode, void *eventData );

/****
 * Time-Variant parameters are double precision vectors (3 element arrays)
 * which may have a time-dependent nature - i.e., the value at time = 1.0
 * is different at time = 2.0. Time-Variant parameters create and destroy
 * LightWave envelopes as needed based on user interaction, application
 * events, etc.
 ****
 * ATTENTION: When using LWVParms with an LWXPanel, the VParm is "get-only"
 *            data. Never update the plugin's pointer during the XPanel
 *            "set". The plugin should always use the LWVParmID returned
 *            by the create method.
 ****
 * API Methods
 * -----------
 *
 * create:      Creates a time-variant parameter instance.
 * destroy:     Releases resources used by instance.
 * setup:       This MUST be called for each parameter created. The envelope
 *              group ID is required. Any envelopes created for the
 *              this parameter will belong to the specified group. A name
 *              is also recommended for the parameter since this is what the
 *              user may see when manipulating the envelope or texture.
 * load:        Loads settings for a Time-Variant Parameter.
 * save:        Saves settings for a Time-Variant Parameter.
 * getVal:      Obtains the value of the parameter at a given time where
 *              result is a 3 element array to be popuplated.
 *              If a texture exists, the LWMicropolID should be non-NULL and
 *              the return value is the texture's opacity.
 *              If, however, the LWMicropolID is NULL, the texture's
 *              contribution is ignored and only the parameter value
 *              itself is returned.
 * setVal:      Sets the value of the parameter where value is an array of
 *              3 doubles. If the paramter is enveloped, setting the value
 *              has no effect. The method returns the number of elements
 *              processed (0, 1, or 3).
 * getState:    Returns a bitflag value containing LWVPSF_* state flags
 *              to indicate true if the parameter has an envelope and/or
 *              texture, and may contain other state information in the future.
 * setState:    This is the counterpart to the getState and using the
 *              bitflags 'state' value to create or destroy the
 *              parameter's envelope and or texture.
 *              A false bit state indicates any existing envelope/texture
 *              should be destroyed so it is recommended to use a getState
 *              to obtain the current state information, make the necessary
 *              adjustments, and then call setState.
 *              Bit states of true will create an envelope or texture
 *              (if one does not already exist) using the information
 *              provided to the setup method.
 * editEnv:     If an envelope exists for the paramenter, this will open
 *              the envelope editor.
 * editTex:     If a texture exists for the paramenter, this will open
 *              the texture editor.
 * initMP:      Utility method to initialize an LWMicropolID instance to
 *              default settings.
 * getEnv:      Populates 'envlist' with envelope instance(s) for the LWVParmID
 *              If no envelope exists, the array elements will be NULL.
 * getTex:      Returns the current texture instance for the LWVParmID
 ****
 */

typedef struct st_LWVParmFuncs {
  LWVParmID    (*create)    ( int envType, int texType );
  void         (*destroy)   ( LWVParmID parm );
  void         (*setup)     ( LWVParmID parm,
                              const char *channelName,
                              LWChanGroupID group,
                              LWTxtrContextID gc,
                              LWVP_EventFunc *eventFunc,
                              const char *pluginName,
                              void *userData );
  LWError       (*copy)     ( LWVParmID to, LWVParmID from );
  LWError       (*load)     ( LWVParmID parm, const LWLoadState *load);
  LWError       (*save)     ( LWVParmID parm, const LWSaveState *save);
  double        (*getVal)   ( LWVParmID parm, LWTime t,
                              LWMicropolID mp, double *result );
  int           (*setVal)   ( LWVParmID parm, double *value );
  int           (*getState) ( LWVParmID parm );
  void          (*setState) ( LWVParmID parm, int state );
  void          (*editEnv)  ( LWVParmID parm );
  void          (*editTex)  ( LWVParmID parm );
  void          (*initMP)   ( LWMicropolID mp );
  void          (*getEnv)   ( LWVParmID parm, LWEnvelopeID envlist[3] );
  LWTextureID   (*getTex)   ( LWVParmID parm );
} LWVParmFuncs;

/* close LWSDK_VPARM_H header */
#endif
