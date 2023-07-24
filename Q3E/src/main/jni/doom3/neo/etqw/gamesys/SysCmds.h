// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SYS_CMDS_H__
#define __SYS_CMDS_H__

void	D_DrawDebugLines( void );
void	Cmd_DoSay( idWStr& text, gameReliableClientMessage_t mode );
void	Cmd_Say( const idCmdArgs &args, gameReliableClientMessage_t mode );

#endif /* !__SYS_CMDS_H__ */
