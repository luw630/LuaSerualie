local tblist = 
{
	--"2000","3000","6000","13000","2200","33000","46000","61000","78000","97000"
	-- itemlist = --通关后的奖励道具列表
	-- {
	--  {range = {1,1},reward = {5,800},},
	--  {range = {2,2},reward = {5,500},},
	--  {range = {3,3},reward = {5,300},},
	--  {range = {4,4},reward = {5,270},},
	--  {range = {5,5},reward = {5,240},},
	--  {range = {6,6},reward = {5,210},},
	--  {range = {7,7},reward = {5,180},},
	--  {range = {8,8},reward = {5,150},},
	--  {range = {9,9},reward = {5,120},},
	--  {range = {10,50},reward = {0,100},},
	-- }, 
		[1] =    --每个关卡信息 1
		{
			BossInfo = {1999010,1609845.6}, --Boss 信息 ID 和血量
			Equipment = {13106,13206,13306}, --通关后的装备道具列表
			Random = {20,60,20},    --通关以后的装备抽取次数，分别递增1，2，3
		},

		[2] =    --每个关卡信息 2
		{
			BossInfo = {1999011,1250313.3}, --Boss 信息 ID 和血量
			Equipment = {13405,13505,13605}, --通关后的装备道具列表
			Random = {20,60,20},    --通关以后的装备抽取次数，分别递增1，2，3
		},
		[3] =    --每个关卡信息 3
		{
			BossInfo = {1999012,1307415.01}, --Boss 信息 ID 和血量
			Equipment = {11106,11206,11306}, --通关后的装备道具列表
			Random = {20,60,20},    --通关以后的装备抽取次数，分别递增1，2，3
		},
}

local FACTIONEXPUPDATE = {2000,3000,6000,13000,2200,33000,46000,61000,78000,97000} --军团升级所需经验 = 1

function look(Obj)
	if type(Obj) ~= "table" then
		rfalse(tostring(Obj))
		return
	end
	local function Save(Obj, Level)
		local Blank = ""
			for i = 1, Level do
				Blank = Blank .. "   "
			end
			for k,v in pairs(Obj) do
				if tostring(k) ~= "" and v ~= Obj then
					if type(v) ~= "table" then
						rfalse(Blank.. " [".. tostring(k).. "] = "..tostring(v))
					else
						rfalse(Blank.. " [".. tostring(k).. "] = {")	
						Save(v, Level + 2)
						rfalse(Blank.."     },")	
					end
				end
			end
	end
	rfalse("   {")
	Save(Obj, 1)
	rfalse("   }")
end


function toSave(  )
	look(tblist)
	return SerializeTable(tblist,1024)
end

function onLoad( tsize)
	rfalse("onLoad")
	local loadtable = {}
	local tload = unSerializeTable(tsize)
	if tload ~= nil then
		look(tload)
	end


end

local tsize = toSave()
onLoad(tsize)