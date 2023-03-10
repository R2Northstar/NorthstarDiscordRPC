use std::{
    sync::Mutex,
    time::{SystemTime, UNIX_EPOCH},
};

use discord_game_sdk::{Activity, Discord, EventHandler};
use rrplug::prelude::*;
use rrplug::wrappers::presence::{GamePresence, GameStateEnum};

const EMPTY: String = String::new();
const APP_ID: i64 = 941428101429231617;

#[derive(Debug, Default, Clone)]
struct ActivityData {
    party: (u32, u32),
    details: String,
    state: String,
    large_image: String,
    large_text: String,
    small_image: String,
    small_text: String,
    end: i64,
    start: i64,
}

#[derive(Debug)]
struct DiscordRpc {
    activity: Option<Mutex<ActivityData>>,
}

impl Plugin for DiscordRpc {
    fn new() -> Self {
        Self { activity: None }
    }

    fn initialize(&mut self, _: &PluginData) {
        self.activity = Some(Mutex::new(ActivityData {
            large_image: "northstar".to_string(),
            state: "Loading...".to_string(),
            ..Default::default()
        }));
    }

    fn main(&self) {
        let mut drpc: Discord<'_, DiscordEvent> = match Discord::new(APP_ID) {
            Ok(drpc) => drpc,
            Err(err) => {
                log::error!("failed to load discord rpc; Is your discord running?");
                log::error!("{err}");
                return;
            }
        };

        drpc.clear_activity(|discord, _| {
            discord.update_activity(
                Activity::empty()
                    .with_state("Loading...")
                    .with_small_image_key("northstar"),
                |_, result| match result {
                    Ok(_) => log::info!("cleared activity"),
                    Err(err) => log::error!("coudln't clear activity because of {:?}", err),
                },
            )
        });

        if let Err(err) = drpc.run_callbacks() {
            #[cfg(debug_assertions)]
            log::error!("failed to run callbacks {err}");
            #[cfg(not(debug_assertions))]
            drop(err)
        }

        loop {
            let data = self.activity.as_ref().unwrap().lock().unwrap().clone();

            drpc.update_activity(
                Activity::empty()
                    .with_details(&data.details)
                    .with_state(&data.state)
                    .with_large_image_key(&data.large_image)
                    .with_small_image_key(&data.small_image)
                    .with_large_image_tooltip(&data.large_text)
                    .with_small_image_tooltip(&data.small_text)
                    .with_end_time(data.end)
                    .with_start_time(data.start)
                    .with_party_amount(data.party.0)
                    .with_party_capacity(data.party.1),
                |_, result| {
                    if let Err(err) = result {
                        #[cfg(debug_assertions)]
                        log::error!("coudln't update activity because of {:?}", err)
                    }
                },
            );

            if let Err(err) = drpc.run_callbacks() {
                #[cfg(debug_assertions)]
                log::error!("failed to run callbacks {err}");
                #[cfg(not(debug_assertions))]
                drop(err)
            }

            wait(1000)
        }
    }

    fn on_presence_updated(&self, presence: &GamePresence) {
        let mut activity = match self.activity.as_ref().unwrap().try_lock() {
            Ok(a) => a,
            Err(_) => return,
        };

        match presence.get_state().unwrap() {
            GameStateEnum::InGame => {}
            _ => {
                let start = SystemTime::now();
                let since_the_epoch = start.duration_since(UNIX_EPOCH).unwrap(); // there is no way this fails
                activity.start = since_the_epoch.as_secs() as i64;
            }
        };

        match presence.get_state().unwrap() {
            GameStateEnum::Loading => {
                activity.party = (0, 0);
                activity.details = "".to_string();
                activity.state = "Loading...".to_string();
                activity.large_image = "northstar".to_string();
                activity.large_text = "Titanfall 2 + Northstar".to_string();
                activity.small_image = EMPTY;
                activity.small_text = EMPTY;
                activity.end = 0;
            }
            GameStateEnum::MainMenu => {
                activity.party = (0, 0);
                activity.details = "Main Menu".to_string();
                activity.state = "On Main Menu".to_string();
                activity.large_image = "northstar".to_string();
                activity.large_text = "Titanfall 2 + Northstar".to_string();
                activity.small_image = EMPTY;
                activity.small_text = EMPTY;
                activity.end = 0;
            }
            GameStateEnum::Lobby => {
                activity.party = (0, 0);
                activity.details = "Lobby".to_string();
                activity.state = "In the Lobby".to_string();
                activity.large_image = "northstar".to_string();
                activity.large_text = "Titanfall 2 + Northstar".to_string();
                activity.small_image = EMPTY;
                activity.small_text = EMPTY;
                activity.end = 0;
            }
            GameStateEnum::InGame => {
                let map_displayname = presence.get_map_displayname();

                activity.party = (
                    presence
                        .get_current_players()
                        .try_into()
                        .unwrap_or_default(),
                    presence.get_max_players().try_into().unwrap_or_default(),
                );
                activity.details = map_displayname.clone();
                activity.state = map_displayname.clone();
                activity.large_image = presence.get_map();
                activity.large_text = map_displayname;
                if presence.get_playlist() == "campaign" {
                    activity.party = (0, 0);
                    activity.end = 0;
                } else {
                    activity.state = presence.get_playlist_displayname().replace("#PL_", "");
                    activity.details = format!(
                        "Score: {} - {} (First to {})",
                        presence.get_own_score(),
                        presence.get_other_highest_score(),
                        presence.get_max_score()
                    );

                    let current_time = SystemTime::now()
                        .duration_since(UNIX_EPOCH)
                        .unwrap()
                        .as_secs() as i64;
                    let ig_end = presence.get_timestamp_end() as i64;
                    activity.end = current_time + ig_end;
                }
            }
        };
    }
}

entry!(DiscordRpc);

#[derive(Default)]
pub struct DiscordEvent;

impl EventHandler for DiscordEvent {}
