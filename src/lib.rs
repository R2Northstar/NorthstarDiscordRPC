use std::{time::{SystemTime, UNIX_EPOCH}, sync::{Mutex, mpsc::{Receiver, channel}}};

use discord_presence::Client;
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
    end: Option<u64>,
    start: Option<u64>,
}

struct DiscordRpc {
    gamestate: Option<GameState>,
    serverinfo: Option<ServerInfo>,
    drpc: Option<Mutex<Client>>,
    recv: Option<Receiver<bool>>
}

impl Plugin for DiscordRpc {
    fn new() -> Self {
        Self {
            gamestate: None,
            serverinfo: None,
            drpc: None,
            recv: None
        }
    }

    fn initialize(&mut self, external_plugin_data: ExternalPluginData) {
        self.gamestate = external_plugin_data.get_game_state_struct();
        self.serverinfo = external_plugin_data.get_server_info_struct();
        println!("[DiscordRPC] discord rpc plugin initialized");

        let mut drpc = Client::new(941428101429231617);
        
        let _ = drpc.start();

        let (send, recv) = channel::<bool>();
        self.recv = Some(recv);
        let send = Mutex::new(send);

        // drpc.block_until_event(discord_presence::Event::Ready).unwrap(); // hmm doesn't work?

        drpc.on_ready(move |_| send.lock().unwrap().send(true).unwrap());

        self.drpc = Some(Mutex::new(drpc));
    }

    fn main(&self) {
        let gamestate = self.gamestate.as_ref().unwrap();
        let serverinfo = self.serverinfo.as_ref().unwrap();
        let mut drpc = self.drpc.as_ref().unwrap().lock().unwrap();
        let recv = self.recv.as_ref().unwrap();
        
        println!("[DiscordRPC] waiting for discord rpc to be ready");
        loop {
            if recv.try_recv().is_ok() {
                println!("[DiscordRPC] discord rpc initialized");
                break;
            }
            wait(1000)
        }

        loop {
            if let Err(err) = drpc.clear_activity() {
                #[cfg(debug_assertions)]
                println!("[DiscordRPC] coudln't clear activity because of {:?}", err)
            }
            
            #[cfg(debug_assertions)]
            println!("[DiscordRPC] started a cycle");

            let playlist = gamestate.playlist();
            let playlist_display_name = gamestate.playlist_display_name();
            let map = gamestate.map();
            let map_display_name = gamestate.map_display_name();
            let loading = gamestate.loading();
            let players = gamestate.players() as u32;
            let max_players = serverinfo.max_players() as u32;

            let mut data = ActivityData {
                party: (0, 0),
                details: EMPTY,
                state: EMPTY,
                large_image: EMPTY,
                large_text: EMPTY,
                small_image: EMPTY,
                small_text: EMPTY,
                end: None,
                start: None,
            };

            if map.is_empty() {
                data = ActivityData {
                    party: (0, 0),
                    details: "Main Menu".to_string(),
                    state: "On Main Menu".to_string(),
                    large_image: "northstar".to_string(),
                    large_text: "Titanfall 2 + Northstar".to_string(),
                    small_image: EMPTY,
                    small_text: EMPTY,
                    end: Some(0),
                    start: None,
                };

                if get_was_in_game() {
                    reset_timer(&mut data);

                    set_was_in_game(false);
                    set_reset_single_player_timer(true);
                }
            } else if map.starts_with("mp_lobby") {
                data = ActivityData {
                    party: (0, 0),
                    details: "Lobby".to_string(),
                    state: "In the Lobby".to_string(),
                    large_image: "northstar".to_string(),
                    large_text: "Titanfall 2 + Northstar".to_string(),
                    small_image: EMPTY,
                    small_text: EMPTY,
                    end: Some(0),
                    start: None,
                };

                if get_was_in_game() {
                    reset_timer(&mut data);

                    set_was_in_game(false);
                    set_reset_single_player_timer(true);
                }
            } else if loading {
                data.party = (players, max_players);
                data.details = "Loading...".to_string();
                if get_was_in_game() {
                    set_was_in_game(false);
                    set_reset_single_player_timer(true);
                }
            } else {
                data.party = (players, max_players);

                if playlist.starts_with("Campaign") {
                    data = ActivityData {
                        party: (0, 0),
                        details: map_display_name,
                        state: playlist_display_name,
                        large_image: EMPTY,
                        large_text: EMPTY,
                        small_image: EMPTY,
                        small_text: EMPTY,
                        end: Some(0),
                        start: None,
                    };

                    if get_reset_single_player_timer() {
                        reset_timer(&mut data);

                        set_was_in_game(false);
                        set_reset_single_player_timer(true);
                    }
                } else {
                    let our_score = gamestate.our_score();
                    let second_highest_score = gamestate.second_highest_score();
                    let highest_score = gamestate.highest_score();
                    let score_limit = serverinfo.score_limit();
                    let endtime = serverinfo.end_time() as u64;

                    data.details = if our_score == highest_score {
                        format!("{} - {}", our_score, second_highest_score)
                    } else {
                        format!("{} - {}", our_score, highest_score)
                    };

                    data.details
                        .push_str(&format!(" (First to {})", score_limit)[..]);
                    if endtime > 0 {
                        let start = SystemTime::now();
                        let since_the_epoch = start.duration_since(UNIX_EPOCH).unwrap();

                        data.end = Some(since_the_epoch.as_secs() + endtime);
                        data.start = Some(0);
                    }
                    set_reset_single_player_timer(false);
                }
                set_was_in_game(true);
            }

            println!("[DiscordRPC] setting the activity");
            if let Err(err) = drpc.set_activity(|act| {
                act.party(|party| party.size(data.party))
                    .details(data.details)
                    .state(data.state)
                    .assets(|asset| {
                        asset
                            .large_image(data.large_image)
                            .small_image(data.small_image)
                            .large_text(data.large_text)
                            .small_text(data.small_text)
                    })
                    .timestamps(|time| {
                        let time = if data.end.is_some() {
                            time.end(data.end.unwrap())
                        } else {
                            time
                        };

                        if data.start.is_some() {
                            time.start(data.start.unwrap())
                        } else {
                            time
                        }
                    })
            }) {
                #[cfg(debug_assertions)]
                println!("[DiscordRPC] coudln't set activity because of {:?}", err)
            }
            
            #[cfg(debug_assertions)]
            println!("[DiscordRPC] completed a cycle");
            wait(100)
        }
    }
}

entry!(DiscordRpc);

fn reset_timer(data: &mut ActivityData) {
    let start = SystemTime::now();
    let since_the_epoch = start.duration_since(UNIX_EPOCH).unwrap(); // there is no way this fails

    let in_ms =
        since_the_epoch.as_secs() * 1000 + since_the_epoch.subsec_nanos() as u64 / 1_000_000;

    data.start = Some(in_ms);
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
