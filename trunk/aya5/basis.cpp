// 
// AYA version 5
//
// ��Ȑ�����s�Ȃ��N���X�@CBasis
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <iostream>
#include <string>
#include <vector>

#if defined(POSIX)
# include <dirent.h>
# include <sys/stat.h>
#endif
/*
#  include <cstdlib>
#  include <sys/types.h>
*/
#include "basis.h"

#include "aya5.h"
#include "ccct.h"
#include "comment.h"
#include "file.h"
#include "function.h"
#include "lib.h"
#include "log.h"
#include "logexcode.h"
#include "messages.h"
#include "misc.h"
#include "parser0.h"
#if defined(POSIX)
# include "posix_utils.h"
#endif
#include "globaldef.h"
#include "wordmatch.h"
#include "wsex.h"
#include "ayavm.h"

/* -----------------------------------------------------------------------
 * CBasis�R���X�g���N�^
 * -----------------------------------------------------------------------
 */

CBasis::CBasis(CAyaVM &vmr) : vm(vmr)
{
	ResetSuppress();
	CreateWordMatch();

	checkparser = 0;
	iolog       = 1;
	msglang     = MSGLANG_JAPANESE;

	dic_charset       = CHARSET_SJIS;
	output_charset    = CHARSET_SJIS;
	file_charset      = CHARSET_SJIS;
	save_charset      = CHARSET_SJIS;
	extension_charset = CHARSET_SJIS;

#if defined(WIN32)
	hlogrcvWnd  = NULL;
#endif
	run         = 0;
}

void CBasis::CreateWordMatch(void)
{
	for(int i = 0; i < FORMULATAG_NUM; i++)
		vm.formulatag_wm().addWord(formulatag[i], i);
}


/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SetModuleHandle
 *  �@�\�T�v�F  ���W���[���n���h�����擾���܂�
 *
 *  ���łɃ��W���[���̎�t�@�C�����擾���s���܂�
 * -----------------------------------------------------------------------
 */
#if defined(WIN32) || defined(_WIN32_WCE)
void	CBasis::SetModuleHandle(HANDLE h)
{
	// ���W���[���n���h�����擾
	hmodule = (HMODULE)h;

	// ���W���[���̎�t�@�C�������擾
	// NT�n�ł͂����Ȃ�UNICODE�Ŏ擾�ł��邪�A9x�n���l������MBCS�Ŏ擾���Ă���UCS-2�֕ϊ�
	char	path[STRMAX];
	GetModuleFileName(hmodule, path, sizeof(path));
	char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
	_splitpath(path, drive, dir, fname, ext);
	std::string	mbmodulename = fname;

	wchar_t	*wcmodulename = Ccct::MbcsToUcs2(mbmodulename, CHARSET_DEFAULT);
	modulename = wcmodulename;
	free(wcmodulename);
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SetPath
 *  �@�\�T�v�F  HGLOBAL�Ɋi�[���ꂽ�t�@�C���p�X���擾���AHGLOBAL�͊J�����܂�
 * -----------------------------------------------------------------------
 */
#if defined(WIN32) || defined(_WIN32_WCE)
void	CBasis::SetPath(yaya::global_t h, int len)
{
	// �擾�Ɨ̈�J��
	std::string	mbpath;
	mbpath.assign((char *)h, 0, len);
	GlobalFree(h);
	// �����R�[�h��UCS-2�֕ϊ��i�����ł̃}���`�o�C�g�����R�[�h��OS�f�t�H���g�j
	wchar_t	*wcpath = Ccct::MbcsToUcs2(mbpath, CHARSET_DEFAULT);
	path = wcpath;
	free(wcpath);
}
#elif defined(POSIX)
void	CBasis::SetPath(yaya::global_t h, int len)
{
    // �擾�Ɨ̈�J��
    path = widen(std::string(h, static_cast<std::string::size_type>(len)));
    free(h);
    // �X���b�V���ŏI����ĂȂ���Εt����B
    if (path.length() == 0 || path[path.length() - 1] != L'/') {
	path += L'/';
    }
    // ���W���[���n���h���̎擾�͏o���Ȃ��̂ŁA�͋Z�ňʒu��m��B
    // ���̃f�B���N�g���ɂ���S�Ă�*.dll(case insensitive)��T���A
    // ���g��aya5.dll�Ƃ�����������܂�ł�����A�����I�ԁB
    // �������Ή�����*.txt���������dll�̒��g�͌����Ɏ��֍s���B
    modulename = L"aya5";
    DIR* dh = opendir(narrow(path).c_str());
    if (dh == NULL) {
		std::cerr << narrow(path) << "is not a directory!" << std::endl;
	exit(1);
    }
    while (true) {
	struct dirent* ent = readdir(dh);
	if (ent == NULL) {
	    break; // ��������
	}
	std::string fname(ent->d_name, strlen(ent->d_name)/*ent->d_namlen*/);	// by umeici. 2005/1/16 5.6.0.232
	if (lc(get_extension(fname)) == "dll") {
		std::string txt_file = narrow(path) + change_extension(fname, "txt");
	    struct stat sb;
	    if (::stat(txt_file.c_str(), &sb) == 0) {
		// txt�t�@�C��������̂ŁA���g�����Ă݂�B
		if (file_content_search(narrow(path) + fname, "aya5.dll") != std::string::npos) {
		    // ����͕���DLL�ł���B
		    modulename = widen(drop_extension(fname));
		    break;
		}
	    }
	}
    }
    closedir(dh);
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SetLogRcvWnd
 *  �@�\�T�v�F  �`�F�b�N�c�[������n���ꂽhWnd��ێ����܂�
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
void	CBasis::SetLogRcvWnd(long hwnd)
{
	hlogrcvWnd = (HWND)hwnd;
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::Configure
 *  �@�\�T�v�F  load���ɍs�������ݒ菈��
 * -----------------------------------------------------------------------
 */
void	CBasis::Configure(void)
{
	// ��b�ݒ�t�@�C���i�Ⴆ��aya.txt�j��ǂݎ��
	std::vector<yaya::string_t>	dics;
	LoadBaseConfigureFile(&dics);
	// ��b�ݒ�t�@�C���ǂݎ��ŏd�ĂȃG���[�����������ꍇ�͂����ŏI��
	if (suppress)
		return;

	// ���M���O���J�n
	vm.logger().Start(logpath, output_charset, msglang, hlogrcvWnd, iolog);

	// �����ǂݍ��݂ƍ\�����
	if (vm.parser0().Parse(dic_charset, dics, loadindex, unloadindex, requestindex))
		SetSuppress();

	{
		CLogExCode logex(vm);

		if (checkparser)
			logex.OutExecutionCodeForCheck();

		// �O��I�����ɕۑ������ϐ��𕜌�
		RestoreVariable();

		if (checkparser)
			logex.OutVariableInfoForCheck();
	}

	// �����܂ł̏����ŏd�ĂȃG���[�����������ꍇ�͂����ŏI��
	if (suppress)
		return;

	// �O�����C�u�����ƃt�@�C���̕����R�[�h��������
	vm.libs().SetCharset(extension_charset);
	vm.files().SetCharset(file_charset);

	// load�֐������s
	ExecuteLoad();

	run = 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::Termination
 *  �@�\�T�v�F  unload���ɍs���I������
 * -----------------------------------------------------------------------
 */
void	CBasis::Termination(void)
{
	// ����}�~����Ă��Ȃ���ΏI�����̏��������s
	if (!suppress) {
		// unload
		ExecuteUnload();
		// ���[�h���Ă��邷�ׂẴ��C�u������unload
		vm.libs().DeleteAll();
		// �J���Ă��邷�ׂẴt�@�C�������
		vm.files().DeleteAll();
		// �ϐ��̕ۑ�
		SaveVariable();
	}

	// ���M���O���I��
	vm.logger().Termination();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::IsSuppress
 *  �@�\�T�v�F  ���݂̎����}�~��Ԃ�Ԃ��܂�
 *
 *  �Ԓl�@�@�F  0/1=��}�~/�}�~
 *
 *  ��b�ݒ�t�@�C���̓ǂݎ��⎫���t�@�C���̉�͒��ɁA����p������ȃG���[�����������
 *  SetSuppress()�ɂ���ė}�~�ݒ肳��܂��B�}�~�ݒ肳���ƁAload/request/unload�ł̓��삪
 *  ���ׂă}�X�N����܂��B���̎��Arequest�̕Ԓl�͏�ɋ󕶎���ɂȂ�܂��B�iHGLOBAL=NULL�A
 *  len=0�ŉ������܂��j
 * -----------------------------------------------------------------------
 */
char	CBasis::IsSuppress(void)
{
	return suppress;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SetSuppress
 *  �@�\�T�v�F  ��������}�~��ݒ肵�܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::SetSuppress(void)
{
	suppress = 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::ResetSuppress
 *  �@�\�T�v�F  ��������}�~�@�\�����Z�b�g���܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::ResetSuppress(void)
{
	suppress = 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::LoadBaseConfigureFile
 *  �@�\�T�v�F  ��b�ݒ�t�@�C����ǂݎ��A�e��p�����[�^���Z�b�g���܂�
 *
 *  ��b�ݒ�t�@�C����DLL�Ɠ��K�w�ɑ��݂��閼�O��"DLL��t�@�C����.txt"�̃t�@�C���ł��B
 *
 *  �����t�@�C���̕����R�[�h��Shift_JIS�ȊO�ɂ�UTF-8��OS�f�t�H���g�̃R�[�h�ɑΉ��ł��܂����A
 *  ���̊�b�ݒ�t�@�C����OS�f�t�H���g�̃R�[�h�œǂݎ���邱�Ƃɒ��ӂ��Ă��������B
 *  ���ۉ��Ɋւ��čl������ꍇ�́A���̃t�@�C�����̋L�q�Ƀ}���`�o�C�g�������g�p����ׂ��ł�
 *  ����܂���i�����R�[�h0x7F�ȉ���ASCII�����݂̂ŋL�q���ׂ��ł��j�B
 * -----------------------------------------------------------------------
 */
void	CBasis::LoadBaseConfigureFile(std::vector<yaya::string_t> *dics)
{
	// �ݒ�t�@�C��("name".txt)�ǂݎ��

	// �t�@�C�����J��
	yaya::string_t	filename = path + modulename + L".txt";
	FILE	*fp = w_fopen(filename.c_str(), L"r");
	if (fp == NULL) {
		SetSuppress();
		return;
	}

	// �ǂݎ�菈��
	CComment	comment;
	yaya::string_t	readline;
	yaya::string_t	cmd, param;

	for (int i = 1; ; i++) {
		// 1�s�ǂݍ���
		if (ws_fgets(readline, fp, dic_charset, 0, i) == WS_EOF)
			break;
		// ���s�͏���
		CutCrLf(readline);
		// �R�����g�A�E�g����
		comment.Process_Top(readline);
		comment.Process(readline);
		comment.Process_Tail(readline);
		// ��s�A�������͑S�̂��R�����g�s�������ꍇ�͎��̍s��
		if (readline.size() == 0)
			continue;
		// �p�����[�^��ݒ�
		if (Split(readline, cmd, param, L",")) {
			SetParameter(cmd, param, dics);
		}
		else {
			vm.logger().Error(E_W, 0, filename, i);
		}
	}

	// �t�@�C�������
	fclose(fp);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SetParameter
 *  �@�\�T�v�F  LoadBaseConfigureFile����Ă΂�܂��B�e��p�����[�^��ݒ肵�܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::SetParameter(yaya::string_t &cmd, yaya::string_t &param, std::vector<yaya::string_t> *dics)
{
	// dic
	if (!cmd.compare(L"dic")) {
		yaya::string_t	filename = path + param;
		dics->push_back(filename);
	}
	// log
	else if (!cmd.compare(L"log")) {
		logpath = path + param;
	}
	// iolog
	else if (!cmd.compare(L"iolog")) {
		if (!param.compare(L"off"))
			iolog = 0;
	}
	// msglang
	else if (!cmd.compare(L"msglang")) {
		if (!param.compare(L"english"))
			msglang = MSGLANG_ENGLISH;
		else
			msglang = MSGLANG_JAPANESE;
	}
	// charset
	else if (!cmd.compare(L"charset")) {
		dic_charset       = Ccct::CharsetTextToID(param.c_str());
		output_charset    = dic_charset;
		file_charset      = dic_charset;
		save_charset      = dic_charset;
		extension_charset = dic_charset;
	}
	// charset
	else if (!cmd.compare(L"charset.dic")) {
		dic_charset       = Ccct::CharsetTextToID(param.c_str());
	}
	else if (!cmd.compare(L"charset.output")) {
		output_charset    = Ccct::CharsetTextToID(param.c_str());
	}
	else if (!cmd.compare(L"charset.file")) {
		file_charset      = Ccct::CharsetTextToID(param.c_str());
	}
	else if (!cmd.compare(L"charset.save")) {
		save_charset      = Ccct::CharsetTextToID(param.c_str());
	}
	else if (!cmd.compare(L"charset.extension")) {
		extension_charset = Ccct::CharsetTextToID(param.c_str());
	}
	// fncdepth
	else if (!cmd.compare(L"fncdepth")) {
		int	f_depth = ws_atoi(param, 10);
		vm.calldepth().SetMaxDepth((f_depth < 2) ? 2 : f_depth);
	}
	// checkparser closed function
	else if (!cmd.compare(L"checkparser")) {
		if (!param.compare(L"on"))
			checkparser = 1;
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::GetParameter
 *  �@�\�T�v�F  �e��p�����[�^�𕶎���ŕԂ��܂�
 * -----------------------------------------------------------------------
 */
yaya::string_t CBasis::GetParameter(const yaya::string_t &cmd)
{
	// log
	if (!cmd.compare(L"log")) {
		return logpath;
	}
	// iolog
	else if (!cmd.compare(L"iolog")) {
		return yaya::string_t(iolog ? L"on" : L"off");
	}
	// msglang
	else if (!cmd.compare(L"msglang")) {
		return yaya::string_t(msglang == MSGLANG_ENGLISH ? L"english" : L"japanese");
	}
	// charset
	else if (!cmd.compare(L"charset")) {
		return Ccct::CharsetIDToTextW(dic_charset);
	}
	// charset
	else if (!cmd.compare(L"charset.dic")) {
		return Ccct::CharsetIDToTextW(dic_charset);
	}
	else if (!cmd.compare(L"charset.output")) {
		return Ccct::CharsetIDToTextW(output_charset);
	}
	else if (!cmd.compare(L"charset.file")) {
		return Ccct::CharsetIDToTextW(file_charset);
	}
	else if (!cmd.compare(L"charset.save")) {
		return Ccct::CharsetIDToTextW(save_charset);
	}
	else if (!cmd.compare(L"charset.extension")) {
		return Ccct::CharsetIDToTextW(extension_charset);
	}
	// fncdepth
	else if (!cmd.compare(L"fncdepth")) {
		yaya::string_t str;
		ws_itoa(str,vm.calldepth().GetMaxDepth(),10);
		return str;
	}
	return L"";
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SaveVariable
 *  �@�\�T�v�F  �ϐ��l���t�@�C���ɕۑ����܂�
 *
 *  �t�@�C������"DLL��t�@�C����_variable.cfg"�ł��B
 *  �t�@�C���t�H�[�}�b�g��1�s1�ϐ��A�f���~�^���p�J���}�ŁA
 *
 *  �ϐ���,���e,�f���~�^
 *
 *  �̌`���ŕۑ�����܂��B���e�͐���/�����̏ꍇ�͂��̂܂܁A������ł̓_�u���N�H�[�g����܂��B
 *  �z��̏ꍇ�͊e�v�f�Ԃ��R�����ŕ�������܂��B�ȉ��ɗv�f��3�A�f���~�^"@"�ł̗�������܂��B
 *
 *  var,1:"TEST":0.3,@
 *
 *  �f���~�^�̓_�u���N�H�[�g����܂���B
 *
 *  ��b�ݒ�t�@�C���Őݒ肵�������R�[�h�ŕۑ�����܂��B
 * -----------------------------------------------------------------------
 */
void	CBasis::SaveVariable(const yaya::char_t* pName)
{
	// �ϐ��̕ۑ�

	// �t�@�C�����J��
	yaya::string_t	filename;
	if ( ! pName || ! *pName ) {
		filename = path + modulename + L"_variable.cfg";
	}
	else {
		filename = path + pName;
	}

	vm.logger().Message(7);
	vm.logger().Filename(filename);
	FILE	*fp = w_fopen((wchar_t *)filename.c_str(), L"w");
	if (fp == NULL) {
		vm.logger().Error(E_E, 57, filename);
		return;
	}
/*
#if defined(WIN32)
	// UTF-8�̏ꍇ�͐擪��BOM��ۑ�
	if (charset == CHARSET_UTF8)
		write_utf8bom(fp);
#endif
	// UTF-8�Ȃ̂�BOM��t����̂͂�߂������X�������Ɓc
	// �g���u���̌����ɂȂ�̂ŁB

	// �����ł��B�O���Ă��܂��܂��B
	// ����
	// UTF-8�ɂ̓o�C�g�I�[�_�[�ɂ��o���G�[�V���������݂��Ȃ��̂�BOM�͕K�v�Ȃ��B
	// �t�^���邱�Ƃ͏o����B�������Ή����Ă��Ȃ��\�t�g�œǂ߂Ȃ��Ȃ�̂ŕt���Ȃ��ق���
	// �ǂ��B
*/

	// �����R�[�h
	fprintf(fp,"//savefile_charset,%s\n",Ccct::CharsetIDToTextA(save_charset));

	// �����ۑ�
	yaya::string_t	wstr;
	char	*tmpstr;
	size_t	var_num = vm.variable().GetNumber();

	for(size_t i = 0; i < var_num; i++) {
		CVariable	*var = vm.variable().GetPtr(i);
		// Void�͕��u
		if (var->value_const().IsVoid())
			continue;
		// ���e���󕶎���̕ϐ��͕ۑ����Ȃ�
		if (var->value_const().IsStringReal() && !var->value_const().s_value.size())
			continue;
		// ���e����ėp�z��̕ϐ��͕ۑ����Ȃ�
//		if (var->value_const().GetType() == F_TAG_ARRAY && !var->value_const().array_size())
//			continue;
		// �����t���O�������Ă���ϐ��͕ۑ����Ȃ�
		if (var->IsErased())
			continue;
		// ���O�̕ۑ�
		tmpstr = Ccct::Ucs2ToMbcs(var->name, save_charset);
		fprintf(fp, "%s,", tmpstr);
		free(tmpstr);
		// �l�̕ۑ�
		switch(var->value_const().GetType()) {
		case F_TAG_INT:
			ws_itoa(wstr, var->value_const().i_value, 10);
			tmpstr = Ccct::Ucs2ToMbcs(wstr, save_charset);
			fprintf(fp, "%s,", tmpstr);
			free(tmpstr);
			break;
		case F_TAG_DOUBLE:
			ws_ftoa(wstr, var->value_const().d_value);
			tmpstr = Ccct::Ucs2ToMbcs(wstr, save_charset);
			fprintf(fp, "%s,", tmpstr);
			free(tmpstr);
			break;
		case F_TAG_STRING:
			wstr = var->value_const().s_value;
			EscapeString(wstr);
			wstr = L"\"" + wstr + L"\"";
			tmpstr = Ccct::Ucs2ToMbcs(wstr, save_charset);
			fprintf(fp, "%s,", tmpstr);
			free(tmpstr);
			break;
		case F_TAG_ARRAY:
			if (!var->value_const().array_size()) {
				fprintf(fp, ESC_IARRAY_SAVE);
				fprintf(fp, ":");
				fprintf(fp, ESC_IARRAY_SAVE);
			}
			else {
				std::vector<CValueSub>::const_iterator	itv;
				std::vector<CValueSub>::const_iterator	itvbegin = var->value_const().array().begin();

				for(itv = itvbegin; itv != var->value_const().array().end(); itv++) {
					if (itv != itvbegin)
						fprintf(fp, ":");
					wstr = itv->GetValueString();
					EscapeString(wstr);
					tmpstr = Ccct::Ucs2ToMbcs(wstr, save_charset);
					if (itv->GetType() == F_TAG_STRING)
						fprintf(fp, "\"%s\"", tmpstr);
					else
						fprintf(fp, "%s", tmpstr);
					free(tmpstr);
				}
				if (var->value_const().array_size() == 1) {
					fprintf(fp, ":");
					fprintf(fp, ESC_IARRAY_SAVE);
				}
			}
			fprintf(fp, ",");
			break;
		default:
			vm.logger().Error(E_W, 7, var->name);
			break;
		};
		// �f���~�^�̕ۑ�
		tmpstr = Ccct::Ucs2ToMbcs(var->delimiter, save_charset);
		fprintf(fp, "%s\n", tmpstr);
		free(tmpstr);
	}

	// �t�@�C�������
	fclose(fp);

	vm.logger().Message(8);
}
/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::RestoreVariable
 *  �@�\�T�v�F  �O��ۑ������ϐ����e�𕜌����܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::RestoreVariable(const yaya::char_t* pName)
{
	// �t�@�C�����J��
	yaya::string_t	filename;
	if ( ! pName || ! *pName ) {
		filename = path + modulename + L"_variable.cfg";
	}
	else {
		filename = path + pName;
	}

	vm.logger().Message(6);
	vm.logger().Filename(filename);
	FILE	*fp = w_fopen((wchar_t *)filename.c_str(), L"r");
	if (fp == NULL) {
		vm.logger().Error(E_N, 0);
		return;
	}

	// ���e��ǂݎ��A�����������Ă���
	yaya::string_t	linebuffer;
	yaya::string_t	readline;
	yaya::string_t	parseline;
	yaya::string_t	varname, value, delimiter;

	char savefile_charset = save_charset;

	for (int i = 1; ; i++) {
		// 1�s�ǂݍ���
		if (ws_fgets(readline, fp, savefile_charset, 0, i, false) == WS_EOF)
			break;
		// ���s�͏���
		CutCrLf(readline);
		// ��s�Ȃ玟�̍s��
		if (readline.size() == 0) {
			linebuffer.erase();
			continue;
		}
		// ���ɓǂݎ��ς̕�����ƌ���
		linebuffer += readline;
		// �_�u���N�H�[�e�[�V���������Ă��邩�m�F����B���Ă��Ȃ��ꍇ�́A
		// ���̍s�֒l�������Ă���Ǝv����̂Ŏ��̍s�̓ǂݎ���
		if (IsInDQ(linebuffer, 0, linebuffer.size() - 1) != IsInDQ_notindq)
			continue;

		parseline = linebuffer;
		linebuffer.erase();

		// �ϐ������擾
		if (!Split_IgnoreDQ(parseline, varname, value, L",")) {
			vm.logger().Error(E_W, 1, filename, i);
			continue;
		}
		// Charset
		if (varname.compare(L"//savefile_charset") == 0) {
			savefile_charset = Ccct::CharsetTextToID(value.c_str());
			continue;
		}
		// �ϐ����̐�����������
		if (IsLegalVariableName(varname)) {
			vm.logger().Error(E_W, 2, filename, i);
			continue;
		}
		// �l�ƃf���~�^�����o��
		parseline = value;
		if (!Split_IgnoreDQ(parseline, value, delimiter, L",")) {
			vm.logger().Error(E_W, 3, filename, i);
			continue;
		}
		// �l���`�F�b�N���Č^�𔻒�
		int	type;
		if (IsIntString(value))
			type = F_TAG_INT;
		else if (IsDoubleString(value))
			type = F_TAG_DOUBLE;
		else if (!IsLegalStrLiteral(value))
			type = F_TAG_STRING;
		else {
			if (value.find(L':') == yaya::string_t::npos) {
				vm.logger().Error(E_W, 4, filename, i);
				continue;
			}
			type = F_TAG_ARRAY;
		}
		// �f���~�^�̐�����������
		if (!delimiter.size()) {
			vm.logger().Error(E_W, 5, filename, i);
			continue;
		}
		// �ϐ����쐬
		int	index = vm.variable().Make(varname, 0);
		vm.variable().SetType(index, type);
		if (type == F_TAG_INT)
			// �����^
			vm.variable().SetValue(index, ws_atoi(value, 10));
		else if (type == F_TAG_DOUBLE) 
			// �����^
			vm.variable().SetValue(index, ws_atof(value));
		else if (type == F_TAG_STRING) {
			// ������^
			CutDoubleQuote(value);
			UnescapeString(value);
			vm.variable().SetValue(index, value);
		}
		else if (type == F_TAG_ARRAY)
			// �z��^
			RestoreArrayVariable(*(vm.variable().GetValuePtr(index)), value);
		else {
			vm.logger().Error(E_W, 6, filename, i);
			continue;
		}
		vm.variable().SetDelimiter(index, delimiter);
	}

	// �t�@�C�������
	fclose(fp);

	vm.logger().Message(8);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::RestoreArrayVariable
 *  �@�\�T�v�F  RestoreVariable����Ă΂�܂��B�z��ϐ��̓��e�𕜌����܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::RestoreArrayVariable(CValue &var, yaya::string_t &value)
{
	var.array().clear();

	yaya::string_t	par, remain;
	char splitResult;

	for( ; ; ) {
		splitResult = Split_IgnoreDQ(value, par, remain, L":");
		if (!splitResult) {
			par = value;
		}

		if (par.compare(ESC_IARRAY) != 0) {
			if (IsIntString(par)) {
				var.array().push_back(CValueSub( ws_atoi(par, 10) ));
			}
			else if (IsDoubleString(par)) {
				var.array().push_back(CValueSub( ws_atof(par) ));
			}
			else {
				CutDoubleQuote(par);
				UnescapeString(par);
				var.array().push_back(CValueSub(par));
			}
		}

		if (!splitResult) {
			break;
		}
		value = remain;
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::ExecuteLoad
 *  �@�\�T�v�F  load�֐������s���܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::ExecuteLoad(void)
{
	if (IsSuppress() || loadindex == -1)
		return;

	// �������idll�̃p�X�j���쐬
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	CValueSub	arg0(path);
	arg.array().push_back(arg0);
	// ���s�@���ʂ͎g�p���Ȃ��̂ł��̂܂܎̂Ă�
	vm.calldepth().Init();
	CLocalVariable	lvar;
	int	exitcode;
	vm.logger().Io(0, path);
	vm.function()[loadindex].Execute(arg, lvar, exitcode);
	yaya::string_t empty;
	vm.logger().Io(1, empty);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::ExecuteRequest
 *  �@�\�T�v�F  request�֐������s���܂�
 * -----------------------------------------------------------------------
 */
#if defined(WIN32) || defined(_WIN32_WCE)
yaya::global_t	CBasis::ExecuteRequest(yaya::global_t h, long *len)
{
	if (IsSuppress() || requestindex == -1) {
		GlobalFree(h);
		*len = 0;
		return NULL;
	}

	// ���͕�������擾
	std::string	istr;
	istr.assign((char *)h, 0, (int)*len);
	
	// �������i���͕�����j���쐬�@�����ŕ����R�[�h��UCS-2�֕ϊ�
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	wchar_t	*wistr = Ccct::MbcsToUcs2(istr, output_charset);
	if (wistr != NULL) {
		CValueSub	arg0 = wistr;
		vm.logger().Io(0, arg0.s_value);
		arg.array().push_back(arg0);
		free(wistr);
	}
	else {
		vm.logger().Io(0, L"");
	}

	// ���s
	vm.calldepth().Init();
	CLocalVariable	lvar;
	int	exitcode;
	CValue	result = vm.function()[requestindex].Execute(arg, lvar, exitcode);

	// ���ʂ𕶎���Ƃ��Ď擾���A�����R�[�h��MBCS�ɕϊ�
	yaya::string_t	res = result.GetValueString();
	vm.logger().Io(1, res);
	char	*mostr = Ccct::Ucs2ToMbcs(res, output_charset);
	if (mostr == NULL) {
		// �����R�[�h�ϊ����s�ANULL��Ԃ�
		*len = 0;
		GlobalFree(h);
		return NULL;
	}

	// �����R�[�h�ϊ������������̂ŁA���ʂ�GMEM�փR�s�[���ĕԂ�
	unsigned int oldlen = *len;
	*len = (long)strlen(mostr);
	unsigned int copylen = *len + 1;

	//�[���I�[���܂ށc�c�݊������̂���
	HGLOBAL r_h = NULL;
	if (oldlen >= copylen) {
		r_h = h;
	}
	else {
		GlobalFree(h);
		r_h = ::GlobalAlloc(GMEM_FIXED,copylen);
	}

	if (r_h) {
		memcpy(r_h, mostr, copylen);
	}
	free(mostr);
	return r_h;
}
#elif defined(POSIX)
yaya::global_t	CBasis::ExecuteRequest(yaya::global_t h, long *len)
{
    if (IsSuppress() || requestindex == -1) {
	free(h);
	*len = 0;
	return NULL;
    }
    
    // ���͕�������擾
	std::string istr(h, *len);
    // �������i���͕�����j���쐬�@�����ŕ����R�[�h��UCS-2�֕ϊ�
    CValue arg(F_TAG_ARRAY, 0/*dmy*/);
    wchar_t *wistr = Ccct::MbcsToUcs2(istr, output_charset);
    if (wistr != NULL) {
	CValueSub arg0 = wistr;
	vm.logger().Io(0, arg0.s_value);
	arg.array().push_back(arg0);
	free(wistr);
    }
    else {
		yaya::string_t empty;
	vm.logger().Io(0, empty);
    }
    
    // ���s
    vm.calldepth().Init();
    CLocalVariable	lvar;
    int	exitcode;
    CValue	result = function[requestindex].Execute(arg, lvar, exitcode);
    // ���ʂ𕶎���Ƃ��Ď擾���A�����R�[�h��MBCS�ɕϊ�
	yaya::string_t	res = result.GetValueString();
    vm.logger().Io(1, res);
    char *mostr = Ccct::Ucs2ToMbcs(res, output_charset);
    if (mostr == NULL) {
	// �����R�[�h�ϊ����s�ANULL��Ԃ�
	*len = 0;
	return NULL;
    }
    // �����R�[�h�ϊ������������̂ŁA���ʂ�GMEM�փR�s�[���ĕԂ�
    *len = (long)strlen(mostr);
    char* r_h = static_cast<char*>(malloc(*len));
    memcpy(r_h, mostr, *len);
    free(mostr);
    return r_h;
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::ExecuteUnload
 *  �@�\�T�v�F  unload�֐������s���܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::ExecuteUnload(void)
{
	if (IsSuppress()|| loadindex == -1)
		return;

	// ���s�@���������@���ʂ͎g�p���Ȃ��̂ł��̂܂܎̂Ă�
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	vm.calldepth().Init();
	CLocalVariable	lvar;
	int	exitcode;
	yaya::string_t empty;
	vm.logger().Io(0, empty);
	vm.function()[unloadindex].Execute(arg, lvar, exitcode);
	vm.logger().Io(1, empty);
}