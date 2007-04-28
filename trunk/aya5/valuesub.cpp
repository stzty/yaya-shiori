// 
// AYA version 5
//
// �z��ɐς܂�Ă���l�������N���X�@CValueSub
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <math.h>

#include "globaldef.h"
#include "value.h"
#include "wsex.h"

/* -----------------------------------------------------------------------
 *  �֐���  �F  CValueSub::GetValueInt
 *  �@�\�T�v�F  �l��int�Ŏ擾���܂�
 *
 *  �Ԓl�@�@�F  0/1/2=�G���[����/�擾�ł���/�擾�ł���(�^���ǂݑւ���ꂽ)
 * -----------------------------------------------------------------------
 */
int	CValueSub::GetValueInt(void) const
{
	switch(type) {
	case F_TAG_INT:
		return i_value;
	case F_TAG_DOUBLE:
		return (int)floor(d_value);
	case F_TAG_STRING:
		return ws_atoi(s_value, 10);
	default:
		return 0;
	};
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CValueSub::GetValueDouble
 *  �@�\�T�v�F  �l��double�Ŏ擾���܂�
 *
 *  �Ԓl�@�@�F  0/1/2=�G���[����/�擾�ł���/�擾�ł���(�^���ǂݑւ���ꂽ)
 * -----------------------------------------------------------------------
 */
double	CValueSub::GetValueDouble(void) const
{
	switch(type) {
	case F_TAG_INT:
		return (double)i_value;
	case F_TAG_DOUBLE:
		return d_value;
	case F_TAG_STRING:
		return ws_atof(s_value);
	default:
		return 0.0;
	};
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CValueSub::GetValue
 *  �@�\�T�v�F  �l��yaya::string_t�ŕԂ��܂�
 * -----------------------------------------------------------------------
 */
yaya::string_t	CValueSub::GetValueString(void) const
{
	switch(type) {
	case F_TAG_INT: {
			yaya::string_t	result;
			ws_itoa(result, i_value, 10);
			return result;
		}
	case F_TAG_DOUBLE: {
			yaya::string_t	result;
			ws_ftoa(result, d_value);
			return result;
		}
	case F_TAG_STRING:
		return s_value;
	default:
		return yaya::string_t(L"");
	};
}

/* -----------------------------------------------------------------------
 *  operator = (int)
 * -----------------------------------------------------------------------
 */
CValueSub &CValueSub::operator =(int value)
{
	type    = F_TAG_INT;
	i_value = value;
	s_value = L"";

	return *this;
}

/* -----------------------------------------------------------------------
 *  operator = (double)
 * -----------------------------------------------------------------------
 */
CValueSub &CValueSub::operator =(double value)
{
	type    = F_TAG_DOUBLE;
	d_value = value;
	s_value = L"";

	return *this;
}

/* -----------------------------------------------------------------------
 *  operator = (yaya::string_t)
 * -----------------------------------------------------------------------
 */
CValueSub &CValueSub::operator =(const yaya::string_t &value)
{
	type    = F_TAG_STRING;
	s_value = value;

	return *this;
}

/* -----------------------------------------------------------------------
 *  operator = (yaya::char_t*)
 * -----------------------------------------------------------------------
 */
CValueSub &CValueSub::operator =(const yaya::char_t *value)
{
	type    = F_TAG_STRING;
	s_value = value;

	return *this;
}

/* -----------------------------------------------------------------------
 *  CalcEscalationTypeNum
 *
 *  �^�̏��i���[���������܂��i���l�D��j
 *  ��{�I��DOUBLE>INT�ł��B
 * -----------------------------------------------------------------------
 */
int CValueSub::CalcEscalationTypeNum(const int rhs) const
{
	int result = type > rhs ? type : rhs;
	if ( result != F_TAG_STRING ) { return result; }

	switch ( type <= rhs ? type : rhs ) {
	case F_TAG_VOID:
	case F_TAG_INT:
		return F_TAG_INT;
	case F_TAG_DOUBLE:
	case F_TAG_STRING:
		return F_TAG_DOUBLE;
	}
	return F_TAG_VOID;
}

/* -----------------------------------------------------------------------
 *  CalcEscalationTypeStr
 *
 *  �^�̏��i���[���������܂��i������D��j
 *  ��{�I��STRING>DOUBLE>INT�ł��B
 * -----------------------------------------------------------------------
 */
int CValueSub::CalcEscalationTypeStr(const int rhs) const
{
	return type > rhs ? type : rhs;
}

/* -----------------------------------------------------------------------
 *  operator + (CValueSub)
 * -----------------------------------------------------------------------
 */
CValueSub CValueSub::operator +(const CValueSub &value) const
{
	int t = CalcEscalationTypeStr(value.type);

	switch(t) {
	case F_TAG_INT:
		return CValueSub(GetValueInt()+value.GetValueInt());
	case F_TAG_DOUBLE:
		return CValueSub(GetValueDouble()+value.GetValueDouble());
	case F_TAG_STRING:
		return CValueSub(GetValueString()+value.GetValueString());
	}

	return CValueSub(value);
}

/* -----------------------------------------------------------------------
 *  operator - (CValueSub)
 * -----------------------------------------------------------------------
 */
CValueSub CValueSub::operator -(const CValueSub &value) const
{
	int t = CalcEscalationTypeNum(value.type);

	switch(t) {
	case F_TAG_INT:
		return CValueSub(GetValueInt()-value.GetValueInt());
	case F_TAG_DOUBLE:
		return CValueSub(GetValueDouble()-value.GetValueDouble());
	}

	return CValueSub(value);
}

/* -----------------------------------------------------------------------
 *  operator * (CValueSub)
 * -----------------------------------------------------------------------
 */
CValueSub CValueSub::operator *(const CValueSub &value) const
{
	int t = CalcEscalationTypeNum(value.type);

	switch(t) {
	case F_TAG_INT:
		return CValueSub(GetValueInt()*value.GetValueInt());
	case F_TAG_DOUBLE:
		return CValueSub(GetValueDouble()*value.GetValueDouble());
	}

	return CValueSub(value);
}

/* -----------------------------------------------------------------------
 *  operator / (CValueSub)
 * -----------------------------------------------------------------------
 */
CValueSub CValueSub::operator /(const CValueSub &value) const
{
	int t = CalcEscalationTypeNum(value.type);

	switch(t) {
	case F_TAG_INT:
		{
			int denom = value.GetValueInt();
			if ( denom ) {
				return CValueSub(GetValueInt() / denom);
			}
			else {
				return CValueSub(GetValueInt());
			}
		}
	case F_TAG_DOUBLE:
		{
			double denom = value.GetValueDouble();
			if ( denom ) {
				return CValueSub(GetValueDouble() / denom);
			}
			else {
				return CValueSub(GetValueDouble());
			}
		}
	}

	return CValueSub(value);
}

/* -----------------------------------------------------------------------
 *  operator % (CValueSub)
 * -----------------------------------------------------------------------
 */
CValueSub CValueSub::operator %(const CValueSub &value) const
{
	int t = CalcEscalationTypeNum(value.type);

	switch(t) {
	case F_TAG_INT:
	case F_TAG_DOUBLE:
		{
			int denom = value.GetValueInt();
			if ( denom ) {
				return CValueSub(GetValueInt() % denom);
			}
			else {
				return CValueSub(GetValueInt());
			}
		}
	}

	return CValueSub(value);
}

/* -----------------------------------------------------------------------
 *  Compare (CValueSub)
 *
 *  operator == �̎��̂ł��B
 *  int��double�̉��Z��double�����ł��Byaya::string_t�Ƃ̉��Z�͋󕶎����Ԃ��܂��B
 * -----------------------------------------------------------------------
 */
int CValueSub::Compare(const CValueSub &value) const
{
	int t = CalcEscalationTypeStr(value.type);

	if (t == F_TAG_INT) {
		return GetValueInt() == value.GetValueInt();
	}
	else if (t == F_TAG_DOUBLE) {
		return GetValueDouble() == value.GetValueDouble();
	}
	else if (t == F_TAG_STRING) {
		return GetValueString() == value.GetValueString();
	}
	else {
		return 0;
	}
}
