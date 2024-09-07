#![deny(non_snake_case)]
//! bindings to squirrel structs

use rrplug::high::squirrel_traits::{GetFromSQObject, GetFromSquirrelVm, PushToSquirrelVm};
use rrplug::prelude::*;

#[derive(
    PushToSquirrelVm, GetFromSquirrelVm, GetFromSQObject, Clone, Copy, Debug, PartialEq, Eq,
)]
#[repr(i32)]
/// binding to GameState
pub enum GameState {
    Loading,
    MainMenu,
    Lobby,
    InGame,
}

#[derive(PushToSquirrelVm, GetFromSquirrelVm, Default, Clone)]
/// binding to GameStateStruct
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
    pub servergamestate: i32,
    pub fd_wavenumber: i32,
    pub fd_totalwaves: i32,
}

#[derive(PushToSquirrelVm, GetFromSquirrelVm, Default, Clone)]
/// binding to UIPresenceStruct
pub struct UIPresenceStruct {
    pub game_state: GameState,
}

impl Default for GameState {
    fn default() -> Self {
        Self::Loading
    }
}

#[derive(Default, Clone, Copy, Debug, PartialEq, Eq)]
#[repr(i32)]
/// binding to ServerGameState
pub enum SVGameState {
    #[default]
    WaitingForCustomStart = 0,
    WaitingForPlayers = 1,
    PickLoadout = 2,
    Prematch = 3,
    Playing = 4,
    SuddenDeath = 5,
    SwitchingSides = 6,
    WinnerDetermined = 7,
    Epilogue = 8,
    Postmatch = 9,
}

impl SVGameState {
    pub fn from_i32(value: i32) -> SVGameState {
        match value {
            0 => SVGameState::WaitingForCustomStart,
            1 => SVGameState::WaitingForPlayers,
            2 => SVGameState::PickLoadout,
            3 => SVGameState::Prematch,
            4 => SVGameState::Playing,
            5 => SVGameState::SuddenDeath,
            6 => SVGameState::SwitchingSides,
            7 => SVGameState::WinnerDetermined,
            8 => SVGameState::Epilogue,
            9 => SVGameState::Postmatch,
            _ => SVGameState::WaitingForCustomStart,
        }
    }
}
