module("luci.controller.engsel", package.seeall)

local BACKEND = "/usr/libexec/engsel-luci"
local TOKENS  = "/etc/engsel/refresh-tokens.json"
local ACTIVE  = "/etc/engsel/active.number"

function index()
	entry({"admin", "services", "engsel"}, template("engsel/main"), _("Engsel"), 80)
	entry({"admin", "services", "engsel", "api"}, call("api_dispatch")).leaf = true
end

-- helpers ---------------------------------------------------------------

local function shell_escape(s)
	if not s then return "''" end
	return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end

local function exec_backend(action, ...)
	local args = { BACKEND, action }
	for _, v in ipairs({...}) do
		args[#args+1] = shell_escape(v)
	end
	local cmd = table.concat(args, " ") .. " 2>/dev/null"
	local p = io.popen(cmd, "r")
	if not p then return nil end
	local out = p:read("*a")
	p:close()
	return out
end

local function json_reply(data)
	local http = require "luci.http"
	http.prepare_content("application/json")
	http.write(data or '{"error":"empty"}')
end

local function read_file(path)
	local f = io.open(path, "r")
	if not f then return nil end
	local d = f:read("*a")
	f:close()
	return d
end

local function write_file(path, data)
	local f = io.open(path, "w")
	if not f then return false end
	f:write(data)
	f:close()
	return true
end

-- read POST body as JSON
local function read_body()
	local http = require "luci.http"
	local raw = http.content()
	if not raw or raw == "" then return {} end
	local json = require "luci.jsonc"
	return json.parse(raw) or {}
end

-- API dispatch -----------------------------------------------------------

function api_dispatch()
	local http = require "luci.http"
	local path = http.getenv("PATH_INFO") or ""
	-- strip /admin/services/engsel/api prefix
	local action = path:match("/admin/services/engsel/api/(.+)") or ""
	action = action:gsub("/", "_")

	local handlers = {
		status          = api_status,
		accounts        = api_accounts,
		request_otp     = api_request_otp,
		submit_otp      = api_submit_otp,
		login           = api_login,
		switch_account  = api_switch_account,
		delete_account  = api_delete_account,
		get_balance     = api_get_balance,
		get_profile     = api_get_profile,
		get_quota       = api_get_quota,
		get_package_detail = api_get_package_detail,
		get_family      = api_get_family,
		buy_balance     = api_buy_balance,
		transaction_history = api_transaction_history,
		store_families  = api_store_families,
		store_packages  = api_store_packages,
		store_segments  = api_store_segments,
		store_redeemables = api_store_redeemables,
		store_notifications = api_store_notifications,
		hot_list        = api_hot_list,
		bookmarks       = api_bookmarks,
		validate_msisdn = api_validate_msisdn,
		circle_group    = api_circle_group,
		famplan_info    = api_famplan_info,
	}

	local fn = handlers[action]
	if fn then
		fn()
	else
		json_reply('{"error":"unknown endpoint","endpoint":"' .. action .. '"}')
	end
end

-- Authenticate: get fresh id_token + access_token for a specific account
local function authenticate(account_idx)
	local json = require "luci.jsonc"
	local tokens_raw = read_file(TOKENS)
	if not tokens_raw then return nil, "no tokens file" end
	local tokens = json.parse(tokens_raw)
	if not tokens or #tokens == 0 then return nil, "no accounts" end

	local idx = account_idx or 0
	if idx >= #tokens then idx = 0 end
	local acct = tokens[idx + 1]
	if not acct or not acct.refresh_token then return nil, "no refresh_token" end

	local result_raw = exec_backend("refresh_token", acct.refresh_token)
	if not result_raw then return nil, "backend error" end
	local result = json.parse(result_raw)
	if not result then return nil, "parse error" end

	if result.id_token and result.access_token then
		-- Update refresh_token in file
		if result.refresh_token then
			acct.refresh_token = result.refresh_token
			write_file(TOKENS, json.stringify(tokens))
		end
		return {
			id_token     = result.id_token,
			access_token = result.access_token,
			number       = acct.number,
			subs_type    = acct.subscription_type or "UNKNOWN",
		}
	end

	return nil, result_raw
end

local function get_active_account_idx()
	local json = require "luci.jsonc"
	local active_num = read_file(ACTIVE)
	if not active_num then return 0 end
	active_num = active_num:match("^%s*(.-)%s*$")
	local tokens_raw = read_file(TOKENS)
	if not tokens_raw then return 0 end
	local tokens = json.parse(tokens_raw)
	if not tokens then return 0 end
	for i, t in ipairs(tokens) do
		if tostring(t.number) == active_num or
		   string.format("%.0f", t.number or 0) == active_num then
			return i - 1
		end
	end
	return 0
end

local function auth_and_call(action, ...)
	local idx = get_active_account_idx()
	local auth, err = authenticate(idx)
	if not auth then
		json_reply('{"error":"auth_failed","detail":"' .. (err or "unknown") .. '"}')
		return
	end
	-- Export tokens as env for the backend
	local env_prefix = string.format(
		"ID_TOKEN=%s ACCESS_TOKEN=%s ",
		shell_escape(auth.id_token),
		shell_escape(auth.access_token)
	)
	local args = { env_prefix .. BACKEND, action }
	for _, v in ipairs({...}) do
		args[#args+1] = shell_escape(v)
	end
	local cmd = table.concat(args, " ") .. " 2>/dev/null"
	local p = io.popen(cmd, "r")
	local out = p and p:read("*a") or '{"error":"exec failed"}'
	if p then p:close() end
	json_reply(out)
end

-- API implementations ---------------------------------------------------

function api_status()
	local json = require "luci.jsonc"
	local idx = get_active_account_idx()
	local tokens_raw = read_file(TOKENS)
	local tokens = tokens_raw and json.parse(tokens_raw) or {}
	local active_num = read_file(ACTIVE)
	active_num = active_num and active_num:match("^%s*(.-)%s*$") or ""

	local accts = {}
	for i, t in ipairs(tokens) do
		accts[i] = {
			number = t.number,
			subscription_type = t.subscription_type or "UNKNOWN",
			active = (i - 1 == idx),
		}
	end

	json_reply(json.stringify({
		accounts = accts,
		active_index = idx,
		active_number = active_num,
		count = #tokens,
	}))
end

function api_accounts()
	local raw = read_file(TOKENS) or "[]"
	json_reply(raw)
end

function api_request_otp()
	local body = read_body()
	local out = exec_backend("request_otp", body.number or "")
	json_reply(out)
end

function api_submit_otp()
	local body = read_body()
	local out = exec_backend("submit_otp", "SMS", body.number or "", body.otp or "")
	json_reply(out)
end

function api_login()
	-- Full login flow: submit OTP, then save account
	local body = read_body()
	local json = require "luci.jsonc"

	local result_raw = exec_backend("submit_otp", "SMS", body.number or "", body.otp or "")
	local result = json.parse(result_raw or "{}")
	if not result or not result.refresh_token then
		json_reply(result_raw or '{"error":"login failed"}')
		return
	end

	-- Get profile to determine subscription type
	local subs_type = "PREPAID"
	local subscriber_id = ""

	-- Temporarily set tokens to get profile
	local env = string.format("ID_TOKEN=%s ACCESS_TOKEN=%s ",
		shell_escape(result.id_token or ""),
		shell_escape(result.access_token or ""))
	local prof_raw = io.popen(env .. BACKEND .. " get_profile 2>/dev/null"):read("*a")
	local prof = json.parse(prof_raw or "{}")
	if prof and prof.data and prof.data.profile then
		subs_type = prof.data.profile.subscription_type or subs_type
		subscriber_id = prof.data.profile.subscriber_id or ""
	end

	-- Save account
	exec_backend("save_account",
		body.number or "0",
		result.refresh_token,
		subs_type,
		subscriber_id)

	-- Set as active
	exec_backend("set_active", body.number or "0")

	json_reply(json.stringify({
		status = "ok",
		number = body.number,
		subscription_type = subs_type,
	}))
end

function api_switch_account()
	local body = read_body()
	exec_backend("set_active", tostring(body.number or ""))
	json_reply('{"status":"ok"}')
end

function api_delete_account()
	local body = read_body()
	exec_backend("delete_account", tostring(body.index or 0))
	json_reply('{"status":"deleted"}')
end

function api_get_balance()   auth_and_call("get_balance") end
function api_get_profile()   auth_and_call("get_profile") end
function api_get_quota()     auth_and_call("get_quota") end

function api_get_package_detail()
	local body = read_body()
	auth_and_call("get_package_detail", body.option_code or "")
end

function api_get_family()
	local body = read_body()
	auth_and_call("get_family",
		body.family_code or "",
		tostring(body.is_enterprise or 0),
		body.migration_type or "NONE")
end

function api_buy_balance()
	local body = read_body()
	auth_and_call("buy_balance",
		body.option_code or "",
		tostring(body.price or 0),
		body.name or "",
		body.token_confirmation or "",
		body.payment_for or "BUY_PACKAGE",
		tostring(body.overwrite_amount or 0))
end

function api_transaction_history()  auth_and_call("transaction_history") end
function api_store_families()
	local body = read_body()
	auth_and_call("store_families", body.subs_type or "PREPAID")
end
function api_store_packages()
	local body = read_body()
	auth_and_call("store_packages", body.subs_type or "PREPAID")
end
function api_store_segments()       auth_and_call("store_segments") end
function api_store_redeemables()    auth_and_call("store_redeemables") end
function api_store_notifications()  auth_and_call("store_notifications") end

function api_hot_list()
	local raw = read_file("/etc/engsel/hot_data/hot.json") or "[]"
	json_reply(raw)
end

function api_bookmarks()
	local raw = read_file("/etc/engsel/bookmark.json") or "[]"
	json_reply(raw)
end

function api_validate_msisdn()
	local body = read_body()
	auth_and_call("validate_msisdn", body.msisdn or "")
end

function api_circle_group()   auth_and_call("circle_group") end
function api_famplan_info()   auth_and_call("famplan_info") end
