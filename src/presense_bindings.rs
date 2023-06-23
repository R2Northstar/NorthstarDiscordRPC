#![deny(non_snake_case)]

use rrplug::high::squirrel_traits::{GetFromSQObject, GetFromSquirrelVm, PushToSquirrelVm};
use rrplug::prelude::*;

#[derive(PushToSquirrelVm, GetFromSquirrelVm, GetFromSQObject, Clone, Copy)]
#[repr(i32)]
pub enum GameState {
    Loading,
    MainMenu,
    Lobby,
    InGame,
}

#[derive(PushToSquirrelVm, GetFromSquirrelVm, Default, Clone)]
pub struct GameStateStruct {
    pub map: String,
    pub map_displayname: String,
    pub playlist: String,
    pub playlist_displayname: String,
    pub current_players: i32,
    pub max_players: i32,
    pub own_score: i32,
    pub other_highest_score: i32,
    pub max_score: i32,
    pub time_end: f32,
}

#[derive(PushToSquirrelVm, GetFromSquirrelVm, Default, Clone)]
pub struct UIPresenceStruct {
    pub game_state: GameState,
}

impl Default for GameState {
    fn default() -> Self {
        Self::Loading
    }
}
