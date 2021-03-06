// 
// AYA version 5
//
// ロギング用クラス　CLog
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <boost/lexical_cast.hpp>

#include "ccct.h"
#include "log.h"
#include "manifest.h"
#include "messages.h"
#include "misc.h"
#if defined(POSIX)
# include "posix_utils.h"
#endif
#include "globaldef.h"
#include "timer.h"
#include "wsex.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::Start
 *  機能概要：  ロギングを開始します
 * -----------------------------------------------------------------------
 */
void	CLog::Start(const yaya::string_t &p, int cs, int ml, HWND hw, char il)
{
	iolog   = il;

	if ( open ) {
		if ( path != p || charset != cs || msglang != ml ) {
			Termination();
		}
		else {
			if ( ! iolog ) {
				Termination();
			}
			return;
		}
	}

	path    = p;
	charset = cs;
	msglang = ml;
	
	if ( hw ) { //hwがある＝玉からの呼び出しなので強制ON、ファイル無効
		path = L"";
	}
	else if ( ! il ) {
		enable = 0;
		return;
	}

#if defined(WIN32)
	// もしhWndがNULLなら起動中のチェックツールを探して取得する
	hWnd    = hw != NULL ? hw : GetCheckerWnd();
#endif

#if defined(POSIX)
	fix_filepath(path);
#endif

	// ロギング有効/無効の判定
	if ( path.size() ) {
		fileen = 1;
		enable = 1;
	}
	else {
		fileen = 0;
#if defined(WIN32)
		if (hWnd == NULL) {
			enable = 0;
			return;
		}
#else
		enable = 0;
		return;
#endif
	}

	// 文字列作成
	yaya::string_t	str = (msglang) ? msge[0] : msgj[0];
	str += GetDateString();
	str += L"\n\n";

	// ファイルへ書き込み
	if (fileen) {
		char	*tmpstr = Ccct::Ucs2ToMbcs(str, charset);
		if (tmpstr != NULL) {
			FILE	*fp = yaya::w_fopen((yaya::char_t *)path.c_str(), L"w");
			if (fp != NULL) {
/*				if (charset == CHARSET_UTF8)
					write_utf8bom(fp);*/
				fprintf(fp, "%s", tmpstr);
				fclose(fp);
			}
			free(tmpstr);
			tmpstr = NULL;
		}
	}
	open = 1;

#if defined(WIN32)
	// チェックツールへ送出　最初に文字コードを設定してから文字列を送出
	if (charset == CHARSET_SJIS)
		SendLogToWnd(L"", E_SJIS);
	else if (charset == CHARSET_UTF8)
		SendLogToWnd(L"", E_UTF8);
	else	// CHARSET_DEFAULT
		SendLogToWnd(L"", E_DEFAULT);

	SendLogToWnd(str, E_I);
#endif
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::Termination
 *  機能概要：  ロギングを終了します
 * -----------------------------------------------------------------------
 */
void	CLog::Termination(void)
{
	if (!enable)
		return;

	Message(1);

	yaya::string_t	str = GetDateString();
	str += L"\n\n";

	Write(str);

	open = 0;

#if defined(WIN32)
	SendLogToWnd(L"", E_END);
#endif
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::Write
 *  機能概要：  ログに文字列を書き込みます
 * -----------------------------------------------------------------------
 */
void	CLog::Write(const yaya::char_t *str, int mode)
{
	if (!enable)
		return;
	if (str == NULL)
		return;
	if (!wcslen(str))
		return;

	// 文字列中の\rは消す
	yaya::string_t	cstr = str;
	int	len = cstr.size();
	for(int i = 0; i < len; ) {
		if (cstr[i] == L'\r') {
			cstr.erase(i, 1);
			len--;
			continue;
		}
		i++;
	}

	// ファイルへ書き込み
	if (fileen) {
		if (! path.empty()) {
			char	*tmpstr = Ccct::Ucs2ToMbcs(cstr, charset);
			if (tmpstr != NULL) {
				FILE	*fp = yaya::w_fopen((yaya::char_t *)path.c_str(), L"a");
				if (fp != NULL) {
					fprintf(fp, "%s", tmpstr);
					fclose(fp);
				}
				free(tmpstr);
				tmpstr = NULL;
			}
		}
	}

#if defined(WIN32)
	// チェックツールへ送出
	SendLogToWnd(cstr, mode);
#endif
}

//----

void	CLog::Write(const yaya::string_t &str, int mode)
{
	Write(str.c_str(), mode);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::Filename
 *  機能概要：  既定のフォーマットでファイル名をログに記録します
 * -----------------------------------------------------------------------
 */
void	CLog::Filename(const yaya::string_t &filename)
{
	yaya::string_t	str =  L"- ";
	str += filename;
	str += L"\n";
	Write(str);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::Message
 *  機能概要：  idで指定された既定のメッセージをログに書き込みます
 * -----------------------------------------------------------------------
 */
void	CLog::Message(int id)
{
	Write((msglang) ? (yaya::char_t *)msge[id] : (yaya::char_t *)msgj[id], 0);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::Error
 *  機能概要：  ログにmodeとidで指定されたエラー文字列を書き込みます
 *
 *  引数　　：　ref         付加情報
 *  　　　　　  dicfilename エラーを起こした箇所を含む辞書ファイルの名前
 *  　　　　　  linecount   エラーを起こした行番号
 *
 *  　　　　　  refとdicfilenameはNULL、linecountは-1とすることで、これらを
 *  　　　　　  非表示にできます
 * -----------------------------------------------------------------------
 */
void	CLog::Error(int mode, int id, const yaya::char_t *ref, const yaya::string_t &dicfilename, int linecount)
{
	if (!enable)
		return;

	// ログに書き込み文字列を作成（辞書ファイル名と行番号）
	yaya::string_t	logstr;
	if (dicfilename.empty())
		logstr = L"-(-) : ";
	else {
		logstr = dicfilename + L"(";
		if (linecount == -1)
			logstr += L"-) : ";
		else {
			logstr += yaya::ws_itoa(linecount);
			logstr += L") : ";
		}
	}
	// ログに書き込み文字列を作成（本文）
	if (msglang) {
		// 英語
		if (mode == E_F)
			logstr += msgfe[id];
		else if (mode == E_E)
			logstr += msgee[id];
		else if (mode == E_W)
			logstr += msgwe[id];
		else
			logstr += msgne[id];
	}
	else {
		// 日本語
		if (mode == E_F)
			logstr += msgfj[id];
		else if (mode == E_E)
			logstr += msgej[id];
		else if (mode == E_W)
			logstr += msgwj[id];
		else
			logstr += msgnj[id];
	}
	// ログに書き込み文字列を作成（付加情報）
	if (ref != NULL) {
		logstr += L" : ";
		logstr += ref;
	}
	// 書き込み
	logstr += L'\n';
	Write(logstr, mode);
}

//----

void	CLog::Error(int mode, int id, const yaya::string_t& ref, const yaya::string_t& dicfilename, int linecount)
{
	Error(mode, id, (yaya::char_t *)ref.c_str(), dicfilename, linecount);
}

//----

void	CLog::Error(int mode, int id, const yaya::char_t *ref)
{
        Error(mode, id, ref, yaya::string_t(), -1);
}

//----

void	CLog::Error(int mode, int id, const yaya::string_t& ref)
{
        Error(mode, id, (yaya::char_t *)ref.c_str(), yaya::string_t(), -1);
}

//----

void	CLog::Error(int mode, int id, const yaya::string_t& dicfilename, int linecount)
{
	Error(mode, id, (yaya::char_t *)NULL, dicfilename, linecount);
}

//----

void	CLog::Error(int mode, int id)
{
        Error(mode, id, (yaya::char_t *)NULL, yaya::string_t(), -1);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::Io
 *  機能概要：  入出力文字列と実行時間をログに記録します
 *  引数　　：  io 0/1=開始時/終了時
 * -----------------------------------------------------------------------
 */
void	CLog::Io(char io, const yaya::char_t *str)
{
	if (!enable || !iolog)
		return;

	static	yaya::timer		timer;

	if (!io) {
		//ignoreiolog機能。
		if(ignore_iolog_strings.size()!=0){
			yaya::string_t cstr=str;
			std::vector<yaya::string_t>::iterator it;

			for(it = ignore_iolog_strings.begin(); it != ignore_iolog_strings.end(); it++){
				if(cstr.find(*it) != yaya::string_t::npos){
					//次の出力のログを抑制する。
					//美しい実装ではないけどbasis側に手を加えたくないので。
					//現状、basis側で必ず入力と出力はワンセットで出るはず
					//-> see basis.cpp ExecuteRequest
					ignore_iolog_noresult=1;
					return;
				}
			}

		}

		Write(L"// request\n");
		Write(str);
		Write(L"\n");

        timer.restart();
	}
	else {
		//ログ抑制
		if(ignore_iolog_noresult){
			ignore_iolog_noresult=0;
			return;
		}

		int elapsed_time = timer.elapsed();
		yaya::string_t t_str = L"// response (Execution time : " + yaya::ws_itoa(elapsed_time) + L"[ms])\n";

		Write(t_str);
		Write(str);
		Write(L"\n");
	}
}

void	CLog::Io(char io, const yaya::string_t &str)
{
	Io(io,str.c_str());
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::IoLib
 *  機能概要：  外部ライブラリ入出力文字列と実行時間をログに記録します
 *  引数　　：  io 0/1=開始時/終了時
 * -----------------------------------------------------------------------
 */
void	CLog::IoLib(char io, const yaya::string_t &str, const yaya::string_t &name)
{
	if (!enable || !iolog)
		return;

	static	yaya::timer		timer;

	if (!io) {
		Write(L"// request to library\n// name : ");
		Write(name + L"\n");
		Write(str + L"\n");

		timer.restart();
	}
	else {
		int elapsed_time = timer.elapsed();
		yaya::string_t t_str = L"// response (Execution time : " + yaya::ws_itoa(elapsed_time) + L"[ms])\n";

		Write(t_str);
		Write(str + L"\n");
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::SendLogToWnd
 *  機能概要：  チェックツールに制御メッセージおよびログ文字列をWM_COPYDATAで送信します
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
void	CLog::SendLogToWnd(const yaya::char_t *str, int mode)
{
	if (hWnd == NULL)
		return;

	COPYDATASTRUCT cds;
	cds.dwData = mode;
	cds.cbData = (wcslen(str) + 1)*sizeof(yaya::char_t);
	cds.lpData = (LPVOID)str;

	DWORD res_dword = 0;
	::SendMessageTimeout(hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds, SMTO_ABORTIFHUNG|SMTO_BLOCK, 5000, &res_dword);
}

//----

void	CLog::SendLogToWnd(const yaya::string_t &str, int mode)
{
	SendLogToWnd((yaya::char_t *)str.c_str(), mode);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::GetCheckerWnd
 *  機能概要：  チェックツールのhWndを取得しますに
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
HWND	CLog::GetCheckerWnd(void)
{
	return FindWindow(CLASSNAME_CHECKTOOL, NULL);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::AddIgnoreIologString
 *  機能概要：  IOログの無視する文字列リストを追加します
 * -----------------------------------------------------------------------
 */
void	CLog::AddIgnoreIologString(const yaya::string_t &ignorestr){
	ignore_iolog_strings.push_back(ignorestr);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLog::AddIgnoreIologString
 *  機能概要：  IOログの無視する文字列リストをクリアします
 * -----------------------------------------------------------------------
 */
void	CLog::ClearIgnoreIologString(){
	ignore_iolog_strings.clear();
}
