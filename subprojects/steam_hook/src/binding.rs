use std::ffi::CString;
use std::ffi::CStr;

//obviously C code doesn't follow rust's conventions
#[repr(C)]
#[derive(PartialEq)]
pub enum error_t { ErrSuccess, ErrFailure }

#[allow(dead_code)]
#[repr(C)]
#[derive(PartialEq)]
pub enum DeploymentErrors_t { Ok, InvalidAppid, GameNotFound, CannotMount, FuseNotInstalled, Bug, CannotSymCopy }

//i have no clue if using these like that is leaking ram.
extern "C" {
	fn deploy(appid: std::os::raw::c_int) -> DeploymentErrors_t;
	fn undeploy(appid: std::os::raw::c_int) -> error_t;
	fn is_deployed(appid: std::os::raw::c_int, status: * mut bool) -> error_t;
	fn get_deploy_target(appid: std::os::raw::c_int) -> * mut std::os::raw::c_char;
	fn get_game_executable(appid: std::os::raw::c_int) -> * mut std::os::raw::c_char;
	fn get_default_game_executable(appid: std::os::raw::c_int) -> * const std::os::raw::c_char;
}

pub fn bind_deploy(appid: i32) -> DeploymentErrors_t {
	unsafe {
		return deploy(appid);
	}
}

pub fn bind_undeploy(appid: i32) -> bool {
	unsafe {
		return undeploy(appid) == error_t::ErrSuccess;
	}
}


pub fn bind_is_deployed(appid: i32) -> Option<bool> {
	let mut status : bool = false;
	unsafe {
		let error = is_deployed(appid, &mut status);
		if error == error_t::ErrFailure {
			return None;
		}
	}
	return Some(status);
}

pub fn bind_get_deploy_target(appid: i32) -> std::path::PathBuf {
	let path:String;
	unsafe {
		let c_path = get_deploy_target(appid);
		let cstr_path: CString = CString::from_raw(c_path);
		path = cstr_path.to_str().expect("Wtf is this ?").to_string();
	}

	return std::path::PathBuf::from(&path);
}

pub fn bind_get_game_executable(appid: i32) -> std::path::PathBuf {
	let path: String;
	unsafe {
		let c_path = get_game_executable(appid);
		let cstr_path: CString = CString::from_raw(c_path);
		path = cstr_path.to_str().expect("Failed to convert cstr to str").to_string();
	}

	return std::path::PathBuf::from(&path);
}


pub fn bind_get_default_game_executable(appid: i32) -> std::path::PathBuf {
	let path: String;
	unsafe {
		let c_path = get_default_game_executable(appid);
		let cstr_path = CStr::from_ptr(c_path);
		path = cstr_path.to_str().expect("Failed to convert cstr to str").to_string();
	}

	return std::path::PathBuf::from(&path);
}
