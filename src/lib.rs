use std::time::{SystemTime, UNIX_EPOCH};

use discord_presence::{Client, DiscordError};
use rrplug::prelude::*;

const EMPTY: String = String::new();

static mut WAS_IN_GAME: bool = false;
static mut RESET_SINGLE_PLAYER_TIMER: bool = true;

struct ActivityData {
    party: (u32, u32),
    details: String,
    state: String,
    large_image: String,
    large_text: String,
    small_image: String,
    small_text: String,
}

struct DiscordRpc {
    gamestate: Option<GameState>,
    serverinfo: Option<ServerInfo>,
}

impl Plugin for DiscordRpc {
    fn new() -> Self {
        Self {
            gamestate: None,
            serverinfo: None,
        }
    }

    fn initialize(&mut self, external_plugin_data: ExternalPluginData) {
        self.gamestate = external_plugin_data.get_game_state_struct();
        self.serverinfo = external_plugin_data.get_server_info_struct();
        println!("discord rpc plugin initialized");
    }

    fn main(&self) {
        let gamestate = self.gamestate.as_ref().unwrap();
        let serverinfo = self.serverinfo.as_ref().unwrap();

        let mut drpc = Client::new(941428101429231617);
        let _ = drpc.start();
        
        loop {
            match drpc.set_activity(|act| act.state("Playing") ) {
                Ok(_) => break,
                Err(err) => match err {
                    DiscordError::NotStarted => wait(1000), // I think this would stop the no discord opened crashes
                    _ => panic!( "the following error prefented discord rpc from starting : {:?}", err ),
                },
            }
        }
        println!("discord rpc initialized");
        

        loop {
            let playlist = gamestate.playlist();
            let playlist_display_name = gamestate.playlist_display_name();
            let map = gamestate.map();
            let map_display_name = gamestate.map_display_name();
            let loading = gamestate.loading();
            let players = gamestate.players() as u32;
            let max_players = serverinfo.max_players() as u32;

            if map.is_empty() {
                let data = ActivityData {
                    party: (0, 0),
                    details: "Main Menu".to_string(),
                    state: "On Main Menu".to_string(),
                    large_image: "northstar".to_string(),
                    large_text: "Titanfall 2 + Northstar".to_string(),
                    small_image: EMPTY,
                    small_text: EMPTY,
                };

                set_activity(&mut drpc, data).expect("Failed to set activity");

                set_end(&mut drpc, 0);

                if get_was_in_game() {
                    reset_timer(&mut drpc);

                    set_was_in_game(false);
                    set_reset_single_player_timer(true);
                }
            } else if map.starts_with("mp_lobby") {
                let data = ActivityData {
                    party: (0, 0),
                    details: "Lobby".to_string(),
                    state: "In the Lobby".to_string(),
                    large_image: "northstar".to_string(),
                    large_text: "Titanfall 2 + Northstar".to_string(),
                    small_image: EMPTY,
                    small_text: EMPTY,
                };

                set_activity(&mut drpc, data).expect("Failed to set activity");

                set_end(&mut drpc, 0);

                if get_was_in_game() {
                    reset_timer(&mut drpc);

                    set_was_in_game(false);
                    set_reset_single_player_timer(true);
                }
            } else {
                if loading {
                    drpc.set_activity(|act| act.party(|party| party.size((players, max_players))))
                        .expect("Failed to set activity");
                    drpc.set_activity(|act| act.details("Loading..."))
                        .expect("Failed to set activity");
                    if get_was_in_game() {
                        set_was_in_game(false);
                        set_reset_single_player_timer(true);
                    }
                } else {
                    drpc.set_activity(|act| act.party(|party| party.size((players, max_players))))
                        .expect("Failed to set activity");

                    if playlist.starts_with("Campaign") {
                        let data = ActivityData {
                            party: (0, 0),
                            details: map_display_name,
                            state: playlist_display_name,
                            large_image: EMPTY,
                            large_text: EMPTY,
                            small_image: EMPTY,
                            small_text: EMPTY,
                        };

                        set_activity(&mut drpc, data).expect("Failed to set activity");

                        set_end(&mut drpc, 0);

                        if get_reset_single_player_timer() {
                            reset_timer(&mut drpc);

                            set_was_in_game(false);
                            set_reset_single_player_timer(true);
                        }
                    } else {
                        let our_score = gamestate.our_score();
                        let second_highest_score = gamestate.second_highest_score();
                        let highest_score = gamestate.highest_score();
                        let score_limit = serverinfo.score_limit();
                        let endtime = serverinfo.end_time() as u64;

                        let mut details = if our_score == second_highest_score {
                            format!("{} - {}", our_score, second_highest_score)
                        } else {
                            format!("{} - {}", our_score, highest_score)
                        };

                        details.push_str(&format!(" (First to {})", score_limit)[..]);
                        drpc.set_activity(|act| act.details(details))
                            .expect("Failed to set activity");
                        if endtime > 0 {
                            let start = SystemTime::now();
                            let since_the_epoch = start.duration_since(UNIX_EPOCH).unwrap();

                            set_end(&mut drpc, since_the_epoch.as_secs() + endtime);
                            set_start(&mut drpc, 0);
                        }
                        set_reset_single_player_timer(false);
                    }
                    set_was_in_game(true);
                }

                wait(10)
            }
        }
    }
}

entry!(DiscordRpc);

fn set_activity(drpc: &mut Client, data: ActivityData) -> Result<(), DiscordError> {
    drpc.set_activity(|act| act.party(|party| party.size(data.party)))?;
    drpc.set_activity(|act| act.details(data.details))?;
    drpc.set_activity(|act| act.state(data.state))?;
    drpc.set_activity(|act| act.assets(|asset| asset.large_image(data.large_image)))?;
    drpc.set_activity(|act| act.assets(|asset| asset.large_text(data.large_text)))?;
    drpc.set_activity(|act| act.assets(|asset| asset.small_image(data.small_image)))?;
    drpc.set_activity(|act| act.assets(|asset| asset.small_text(data.small_text)))?;

    Ok(())
}

fn set_end(drpc: &mut Client, t: u64) {
    drpc.set_activity(|act| act.timestamps(|time| time.end(t)))
        .expect("Failed to set activity");
}

fn set_start(drpc: &mut Client, t: u64) {
    drpc.set_activity(|act| act.timestamps(|time| time.start(t)))
        .expect("Failed to set activity");
}

fn reset_timer(drpc: &mut Client) {
    let start = SystemTime::now();
    let since_the_epoch = start.duration_since(UNIX_EPOCH).unwrap(); // there is no way this fails

    let in_ms =
        since_the_epoch.as_secs() * 1000 + since_the_epoch.subsec_nanos() as u64 / 1_000_000;
    drpc.set_activity(|act| act.timestamps(|time| time.start(in_ms)))
        .expect("Failed to set activity");
}

fn get_was_in_game() -> bool {
    unsafe { WAS_IN_GAME }
}

fn set_was_in_game(wig: bool) {
    unsafe { WAS_IN_GAME = wig }
}

fn get_reset_single_player_timer() -> bool {
    unsafe { RESET_SINGLE_PLAYER_TIMER }
}

fn set_reset_single_player_timer(val: bool) {
    unsafe { RESET_SINGLE_PLAYER_TIMER = val }
}
