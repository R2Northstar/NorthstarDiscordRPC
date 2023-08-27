#![allow(non_snake_case)]

use parking_lot::Mutex;
use rrplug::prelude::*;
use tokio::runtime::Runtime;

use crate::{
    discord::async_main,
    presense_bindings::{GameStateStruct, UIPresenceStruct,GameState},
    presense::run_presence_updates,
};

pub(crate) mod discord;
pub(crate) mod presense;
pub(crate) mod presense_bindings;
pub(crate) mod utils;

#[deny(non_snake_case)]
#[derive(Debug, Default, Clone)]
pub struct ActivityData {
    party: Option<(u32, u32)>,
    details: String,
    state: String,
    large_image: Option<String>,
    large_text: Option<String>,
    small_image: Option<String>,
    small_text: Option<String>,
    end: Option<i64>,
    start: Option<i64>,
    last_state: GameState,
}

#[deny(non_snake_case)]
pub struct DiscordRpcPlugin {
    pub activity: Mutex<ActivityData>,
    pub presence_data: Mutex<(GameStateStruct, UIPresenceStruct)>,
}

#[deny(non_snake_case)]
impl Plugin for DiscordRpcPlugin {
    fn new(plugin_data: &PluginData) -> Self {
        plugin_data.register_sq_functions(presense::fetch_presence);

        let activity = Mutex::new(ActivityData {
            large_image: Some("northstar".to_string()),
            state: "Loading...".to_string(),
            ..Default::default()
        });
        Self {
            activity,
            presence_data: Mutex::new((GameStateStruct::default(), UIPresenceStruct::default())),
        }
    }

    fn main(&self) {
        let runtime = match Runtime::new() {
            Ok(rt) => rt,
            Err(err) => {
                log::error!("failed to create a runtime; {:?}", err);
                return;
            }
        };
        runtime.block_on(async_main());
    }

    fn on_sqvm_created(&self, sqvm_handle: &CSquirrelVMHandle) {
        match sqvm_handle.get_context() {
            ScriptVmType::Client | ScriptVmType::Ui => {
                run_presence_updates(unsafe { sqvm_handle.get_sqvm() })
            }
            _ => {}
        }
    }
}

entry!(DiscordRpcPlugin);
