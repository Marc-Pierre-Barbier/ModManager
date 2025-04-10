use std::env;
mod binding;
use std::process::Command;

use crate::binding::{bind_deploy, bind_undeploy, DeploymentErrors_t};

fn replace_path_at_common(path: std::path::PathBuf, new_path: std::path::PathBuf) -> std::path::PathBuf {
	let base: std::path::PathBuf = path.components()
		.skip_while(|component| component.as_os_str() != "common")
		.skip(2) //remove common and the game dir
		.collect();

	return new_path.join(base);
}


fn main() {
	let steam_appid_string = env::args()
		.rfind(|f| f.contains("AppId="))
		.expect("[MOD_MANAGER] This is not steam, We are expecting to see AppId=VALUE in the args");

	let appid_string = steam_appid_string.split("AppId=")
		.nth(1)
		.expect("[MOD_MANAGER] Invalid appid format");

	let appid: i32 = appid_string.parse()
		.expect("[MOD_MANAGER] Invalid appid format");

	let is_deployed = binding::bind_is_deployed(appid)
		.expect("[MOD_MANAGER] Failed to fetch the status");

	let game_path_index = env::args()
		.position(|f| f == "waitforexitandrun")
		.expect("[MOD_MANAGER] Failed to find GamePath index") + 1;

	//The path where the game expect to start
	let steam_game_path = std::path::PathBuf::from(env::args()
		.nth(game_path_index)
		.expect("[MOD_MANAGER] Could not extract game path")
	);

	let default_game_executable = binding::bind_get_default_game_executable(appid);
	//assert_eq!(
	//	default_game_executable,
	//	steam_game_path.file_name()
	//		.expect("[MOD_MANAGER]Failed to extract game executable")
	//);

	//The path where we deployed
	let game_path = binding::bind_get_deploy_target(appid);
	let game_executable = binding::bind_get_game_executable(appid);
	println!("[MOD_MANAGER] Debug: {} -- {} -- {}", default_game_executable.to_str().expect("fuck this"), game_path.to_str().expect("fuck this"), game_executable.to_str().expect("fuck this"));

	let mut new_path = replace_path_at_common(steam_game_path, game_path.clone());
	new_path.set_file_name(game_executable);

	//replace the arg where the path is stored by our altered path
	let mut child_args: Vec<_> = env::args()
		.skip(1)
		.collect(); //skip ourselves

	child_args[game_path_index - 1] = new_path.to_str()
		.expect("[MOD_MANAGER] Failed to process game args").to_string();

	println!("[MOD_MANAGER] Edited game args: {}", child_args.join(" "));
	println!("[MOD_MANAGER] Current deployment status: {}", is_deployed);


	if !is_deployed {
		println!("[MOD_MANAGER] Requesting deployment");
		let error = bind_deploy(appid);
		match error {
			DeploymentErrors_t::Bug => println!("[MOD_MANAGER] An unknown error occured, please report the issue"),
			DeploymentErrors_t::CannotMount => println!("[MOD_MANAGER] Failed to mount the overlay filesystem"),
			DeploymentErrors_t::CannotSymCopy => println!("[MOD_MANAGER] Failed to crate the virtual game folder"),
			DeploymentErrors_t::FuseNotInstalled => println!("[MOD_MANAGER] fuse-overlayfs is not installed"),
			DeploymentErrors_t::GameNotFound => println!("[MOD_MANAGER] Failed to find the game, please report this issue"),
			DeploymentErrors_t::InvalidAppid => println!("[MOD_MANAGER] The game is not supported"),
			DeploymentErrors_t::Ok => println!("[MOD_MANAGER] Starting game")
		}

		if error != DeploymentErrors_t::Ok {
			std::process::exit(1);
		}
	}

	let steam_launcher = child_args.remove(0);
	let exit_code = Command::new(steam_launcher)
		.args(child_args)
		.current_dir(game_path)
		.spawn()
		.expect("[MOD_MANAGER] Failed to run the child")
		.wait()
		.expect("[MOD_MANAGER] Failed to wait for the child");

	//force closing a game might leave the folder deployed
	//so we always unbind to be safe
	bind_undeploy(appid);

	std::process::exit(exit_code.code().expect("[MOD_MANAGER] Failed to get game exit code"));
}
