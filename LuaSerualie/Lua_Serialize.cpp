// Lua_Serialize.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
};

#pragma comment(lib, "lua5.1.lib")

byte  *g_buffstream=0;
int L_rfalse(lua_State *L)
{
	LPCSTR str = lua_tostring( L, 1 );
	static char hugestr[10240];
	sprintf_s( hugestr, "%s\r\n", str );
	OutputDebugString( hugestr );
	printf(hugestr);
	return 0;
}

int unSerializeTable(lua_State* L, byte* stream,int nsize)
{
	if (!lua_istable(L,-1))
	{
		return -1;
	}
	byte* pv_stm = stream;
	DWORD key_type = 0;
	DWORD value_type = 0;
	DWORD value_len = 0;
	DWORD key_len = 0;

	char szKey[100] = { 0 };
	char szValue[100] = { 0 };
	DWORD dkey = 0;
	DWORD  dValue = 0;

	int nIndex = 0;
	while (1)
	{
		if (pv_stm - stream >= nsize)
		{
			break;
		}

		memcpy(&key_type, pv_stm, 1);
		pv_stm++;
		if ((char)key_type == 0x4a)
		{
			break;
		}

		value_type = ((key_type >> 1) & 7);
		key_len = ((key_type>>4) & 3);
		value_len = ((key_type>>6) & 3);

		memset(szKey, 0, 100);
		memset(szValue, 0, 100);
		dkey = 0;
	   dValue = 0;
		
		if (key_type & 1)
		{
			memcpy(&dkey, pv_stm, key_len + 1);
			pv_stm += (key_len + 1);
			
			if (value_type == 5)
			{
				lua_pushnumber(L, dkey);
				lua_createtable(L, 0, 0);
				int unsize = unSerializeTable(L, pv_stm, nsize - (pv_stm - stream));
				if (unsize > 0)
				{
					lua_rawset(L, -3);
					pv_stm += unsize;
				}
			}
			else if (value_type == 1)
			{
				memcpy(&dValue, pv_stm, value_len + 1);
				pv_stm += (value_len + 1);
				lua_pushnumber(L, dValue);
				lua_rawseti(L, -2, dkey);
			}
			else if (value_len > 0)
			{
				strcpy(szValue, (char*)pv_stm);
				pv_stm += strlen(szValue) + 1;
				lua_pushstring(L, szValue);
				lua_rawseti(L, -2, dkey);
			}

		}
		else if (key_len > 0)
		{
			strcpy(szKey, (char*)pv_stm);
			pv_stm += strlen(szKey) + 1;
	
			if (value_type ==5)
			{
				lua_pushstring(L, szKey);
				lua_createtable(L, 0, 0);
				int unsize = unSerializeTable(L, pv_stm, nsize - (pv_stm - stream));
				if (unsize > 0)
				{
					lua_settable(L, -3);
					pv_stm += unsize;
					//lua_rawset(L, -4);
					//lua_setfield(L, -3, szKey);
					//lua_settable(L, -3);
				}
			}
			else if (value_type ==1)
			{
				memcpy(&dValue, pv_stm, value_len + 1);
				pv_stm += (value_len + 1);
				lua_pushstring(L, szKey);
				lua_pushnumber(L, dValue);
				lua_settable(L, -3);
			}
			else if (value_len > 0)
			{
				strcpy(szValue, (char*)pv_stm);
				pv_stm += strlen(szValue) + 1;
				lua_pushstring(L, szKey);
				lua_pushstring(L, szValue);
				lua_settable(L, -3);
			}
		}
	}
	return (pv_stm - stream);
}

int SerializeTable( lua_State* L,  int index, byte* stream)
{
	byte* pv_stm = stream;
	lua_pushnil(L); /* 第一个 key */
	long ret = -1;

	while (lua_next(L, index) != 0)
	{		
		DWORD lead_byte = 0;
		DWORD key_type = 0;
		DWORD key_len = 0;
		DWORD value_type = 0;
		DWORD value_len = 0;

		char szKey[100] = {0};
		char szValue[100] = {0};		
		int iKey = 0;
		int dValue = 0;

		int lua_kt = lua_type(L, -2);		// key 的LUA类型
		int lua_vt = lua_type(L, -1);		// value 的LUA类型
		switch(lua_kt)
		{
		case LUA_TSTRING:
			{
				key_type = 0;				
				/*szKey = lua_tostring(L, -2);	*/			
				key_len = lua_objlen(L, -2);
				strncpy_s(szKey, lua_tostring(L, -2), key_len);
				if (key_len == 0)
					continue;
				if (key_len >= 4)
					key_len = 3;
				else
					key_len--;
			}
			break;
		case LUA_TNUMBER:			// KEY 只支持整数
			{
				key_type = 1;
				iKey = lua_tointeger(L, -2);
				//if ( v >= 0 ) v++; 这样可以使得客户端AS里边的数组下标满足服务器lua逻辑!
				if ((iKey & 0xffffff00) == 0) key_len = 0;
				else if ((iKey & 0xffff0000) == 0) key_len = 1;
				else if ((iKey & 0xff000000) == 0) key_len = 2;
				else key_len = 3;
			}
			break;
		default:
			return -5;
		}

		switch(lua_vt)
		{
		case LUA_TSTRING:
			{
				value_type = 0;
				/*szValue = lua_tostring(L, -1);	*/
				value_len = lua_objlen(L, -1);
				strncpy_s(szValue, lua_tostring(L, -1), value_len);
				if (value_len != 0)
				{
					if (value_len >= 4)
						value_len = 3;
					else
						value_len--;
				}
			}
			break;
		/*case LUA_TNUMBER:
			{
				dValue = lua_tonumber(L, -1);
				value_type = 2;
				value_len = 3;
			}*/
		case LUA_TNUMBER:
			{
				value_type = 1;
				dValue = lua_tointeger(L, -1);
				if ((dValue & 0xffffff00) == 0) value_len = 0;
				else if ((dValue & 0xffff0000) == 0) value_len = 1;
				else if ((dValue & 0xff000000) == 0) value_len = 2;
				else value_len = 3;

				//dValue = lua_tointeger(L, -1);
				//value_type = 2;
				//value_len = 3;
			}
			break;
		case LUA_TTABLE:
			{
				value_type = 5;
				value_len = 0;
			}
			break;
		default:
			return -5;
		}



		// 键对检测通过，开始写值
		lead_byte = key_type | (value_type << 1) | (key_len << 4) | (value_len << 6);

		memcpy(pv_stm, (char*)&lead_byte, 1);
		pv_stm  = pv_stm + 1;

		if (key_type == 0)
		{
			//memcpy(pv_stm,szKey,key_len);
			//pv_stm = pv_stm + key_len;
			strncpy((char*)pv_stm,szKey, strlen(szKey));
			pv_stm = pv_stm + strlen(szKey);
			if (strlen(szKey) == 0 || key_len == 3)
			{
				memset(pv_stm,0,1); // 为不定长的字符串以及空字符串写入结束符!
				pv_stm++;
			}
		}
		else
		{
			memcpy(pv_stm,(char*)&iKey,4);
			pv_stm = pv_stm + 4;
			pv_stm -= (3 - key_len);
		}

		switch (value_type)
		{
		case 0:
			{
				//memcpy(pv_stm,szValue,value_len);
				//pv_stm = pv_stm + value_len;
				strncpy((char*)pv_stm, szValue, strlen(szValue));
				pv_stm = pv_stm + strlen(szValue);
				if (strlen(szValue) == 0 || value_len == 3)
				{
					memset(pv_stm,0,1); // 为不定长的字符串以及空字符串写入结束符!
					pv_stm++;
				}
			}	
			break;
		/*case 2:
			{
				memcpy(pv_stm,(char*)&dValue,8);
				pv_stm = pv_stm + 8;
			}*/
		case 1:
			{
				memcpy(pv_stm,(char*)&dValue,4);
				pv_stm = pv_stm + 4;
				pv_stm -= (3 - value_len);
			}
			break;
		case 5:
			{
				int top = lua_gettop(L);
				ret = SerializeTable(L, top, pv_stm);
				if (ret < 0)
					return ret;

				pv_stm = pv_stm + ret;
				// 压入表结束符
		
				//memset(pv_stm,'\\',1); 
				memset(pv_stm, 0x4a, 1);
				pv_stm++;
			}
			break;
		default:
			return - 2;
		}

		lua_pop(L, 1);//pop the top element
	}

	ret = pv_stm - stream;
	return ret;
}

int L_SerializeTable(lua_State *L)
{
	if (!g_buffstream)
	{
		int streamsize = luaL_checknumber(L, 2);
		g_buffstream = new byte[streamsize];
		memset(g_buffstream, 0, streamsize);
		//lua_pop(L, 1);
		//lua_settop(L, 2);
	}
// 	g_buffstream = new byte[1024];
// 	memset(g_buffstream, 0, 1024);
	if (lua_istable(L,1))
	{
		int bufsize = SerializeTable(L, 1, g_buffstream);
		if (bufsize)
		{
			lua_pushnumber(L, bufsize);
			return 1;
		}
	}
	return 1;
}

int L_unSerializeTable(lua_State *L)
{
	int ntablesize = luaL_checknumber(L, 1);
	lua_createtable(L, 0, 0);
	if (!lua_istable(L, -1))
	{
		return 0;
	}
	int nunsize = unSerializeTable(L, g_buffstream, ntablesize);
	if (nunsize < 0)
	{
		return 0;
	}
	return 1;
	
}


int _tmain(int argc, _TCHAR* argv[])
{
	double dv = 1.5;
	int nv = 15;
	lua_State* L;
	L = lua_open();        //<初始化lua对象指针>
	luaL_openlibs(L);       //<加载lua函数库>
	lua_register(L, "rfalse", L_rfalse);
	lua_register(L, "SerializeTable", L_SerializeTable);
	lua_register(L, "unSerializeTable", L_unSerializeTable);
	int ret = luaL_dofile(L, "Serialize.lua");
	if(ret != 0)
	{
		OutputDebugString( lua_tostring(L, -1) );
		return false;
	}	

// 	lua_getglobal( L, "toSave");
// 	if (lua_isnil(L, -1)) 
// 	{
// 		lua_pop(L, 1);
// 		return 0; 
// 	}
// 	if (lua_pcall(L, 0, 0, 0) != 0)
// 	{
// 		char temp[1024];
// 		sprintf(temp, "%s\r\n", lua_tostring(L, -1));
// 		printf(temp);
// 		lua_settop(L, 0);
// 		return 0;
// 	}

// 	double iii = 3.000;
// 	std::cout << typeid(iii).name();

// 	byte buf[1024] = {0};
// 	int index = lua_gettop(L);
// 
// 	int bufsize = SerializeTable(L, index, buf);
// 
// 	printf("bufsize = %d", bufsize);
// 
// 	lua_newtable(L);
// 	int unsize = unSerializeTable(L, buf, bufsize);


	lua_close(L);

	system("pause");

	return 0;
}

