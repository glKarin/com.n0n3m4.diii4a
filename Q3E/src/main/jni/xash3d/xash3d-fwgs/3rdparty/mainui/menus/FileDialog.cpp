/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "Framework.h"
#include "Bitmap.h"
#include "Action.h"
#include "Table.h"
#include "StringVectorModel.h"
#include "PicButton.h"

#define ART_BANNER	  	"gfx/shell/head_touchoptions"
#define ART_GAMMA		"gfx/shell/gamma"

uiFileDialogGlobal_t	uiFileDialogGlobal;

class CMenuFileDialog : public CMenuFramework
{
public:
	CMenuFileDialog() : CMenuFramework("CMenuFileDialog") { }

private:
	void _Init( void ) override;
	void _VidInit( void ) override;
	void SaveAndPopMenu() override;
	void RejectChanges();
	void ApplyChanges( const char *fileName );
	void UpdateExtra();

	class CFileListModel : public CStringVectorModel
	{
	public:
		CFileListModel() : CStringVectorModel() {}
		void Update() override;
	} model;

	CMenuTable fileList;

	class CPreview : public CMenuAction
	{
	public:
		void Draw() override;
		HIMAGE image;
	} preview;
};

void CMenuFileDialog::CPreview::Draw()
{
	UI_FillRect( m_scPos.x - 2, m_scPos.y - 2,  m_scSize.w + 4, m_scSize.h + 4, 0xFFC0C0C0 );
	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h, 0xFF808080 );
	EngFuncs::PIC_Set( image, 255, 255, 255, 255 );
	EngFuncs::PIC_DrawTrans( m_scPos, m_scSize );
}

void CMenuFileDialog::CFileListModel::Update( void )
{
	char	**filenames;
	int	i = 0, numFiles, j, k;


	for( k = 0; k < uiFileDialogGlobal.npatterns; k++)
	{
		filenames = EngFuncs::GetFilesList( uiFileDialogGlobal.patterns[k], &numFiles, TRUE );
		for ( j = 0; j < numFiles; i++, j++ )
			AddToTail( filenames[j] );
	}

}

void CMenuFileDialog::ApplyChanges(const char *fileName)
{
	Q_strncpy( uiFileDialogGlobal.result, fileName, sizeof( uiFileDialogGlobal.result ) );
	uiFileDialogGlobal.result[255] = 0;
	uiFileDialogGlobal.valid = false;
	uiFileDialogGlobal.callback( fileName[0] != 0 );
}

void CMenuFileDialog::RejectChanges()
{
	ApplyChanges( "" );
	Hide();
}

void CMenuFileDialog::SaveAndPopMenu()
{
	const char *fileName = model.GetText( fileList.GetCurrentIndex() );
	ApplyChanges( fileName );
	Hide();
}

void CMenuFileDialog::UpdateExtra()
{
	const char *fileName = model.GetText( fileList.GetCurrentIndex() );
	if( uiFileDialogGlobal.preview )
		preview.image = EngFuncs::PIC_Load( fileName );
}

/*
=================
UI_FileDialog_Init
=================
*/
void CMenuFileDialog::_Init( void )
{
	// banner.SetPicture( ART_BANNER );

	fileList.iFlags |= QMF_DROPSHADOW;
	fileList.SetModel( &model );
	fileList.onChanged = VoidCb( &CMenuFileDialog::UpdateExtra );
	fileList.SetRect( 360, 230, -20, 465 );
	UpdateExtra();

	preview.SetRect( 72, 380, 196, 196 );

	// AddItem( banner );
	AddButton( L( "Done" ), L( "Use selected file" ), PC_DONE, VoidCb( &CMenuFileDialog::SaveAndPopMenu ) );
	AddButton( L( "GameUI_Cancel" ), L( "Cancel file selection" ), PC_CANCEL, VoidCb( &CMenuFileDialog::RejectChanges ) );
	AddItem( preview );
	AddItem( fileList );
}

void CMenuFileDialog::_VidInit()
{
	preview.SetVisibility( uiFileDialogGlobal.preview );
}

ADD_MENU( menu_filedialog, CMenuFileDialog, UI_FileDialog_Menu );
