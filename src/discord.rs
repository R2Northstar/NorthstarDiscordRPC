#![deny(non_snake_case)]

use std::num::NonZeroU32;

use discord_sdk::{
    activity::{ActivityBuilder, Assets, PartyPrivacy},
    user::User,
    wheel::UserState,
    wheel::Wheel,
    Discord, DiscordApp, Subscriptions,
};
use rrplug::prelude::*;

use crate::exports::PLUGIN;

/// the discord app's id, taken from older v1 discord rpc
const APP_ID: i64 = 941428101429231617;

pub struct Client {
    pub discord: Discord,
    pub user: User,
    pub wheel: Wheel,
}

pub async fn async_main() {
    let activity = &PLUGIN.wait().activity;

    let client = match make_client(Subscriptions::ACTIVITY).await {
        Ok(c) => c,
        Err(_) => {
            log::error!("Is your discord running?");
            return;
        }
    };

    match client.discord.clear_activity().await {
        Ok(_) => log::info!("cleared activity"),
        Err(err) => log::error!("coudln't clear activity because of {:?}", err),
    }

    loop {
        let data = activity.lock().clone();

        // updates presence here
        if let Err(err) = client
            .discord
            .update_activity(
                ActivityBuilder::default()
                    .details(data.details)
                    .state(data.state)
                    .assets(Assets {
                        large_image: data.large_image,
                        large_text: data.large_text,
                        small_image: data.small_image,
                        small_text: data.small_text,
                    })
                    .start_timestamp(if data.start == 0 { 1 } else { data.start })
                    .end_timestamp(data.end)
                    .party(
                        "whar",
                        Some(
                            data.party
                                .0
                                .try_into()
                                .unwrap_or(NonZeroU32::new(1).unwrap()),
                        ),
                        Some(
                            data.party
                                .1
                                .try_into()
                                .unwrap_or(NonZeroU32::new(1).unwrap()),
                        ),
                        PartyPrivacy::Private,
                    ),
            )
            .await
        {
            log::info!("failed to updated discord activity; {err}");
            #[cfg(not(debug_assertions))]
            return;
        }

        wait(1000);
    }
}

/// discord connection init sourced from https://github.com/EmbarkStudios/discord-sdk/blob/d311db749b7e11cc55cb1a9d7bfd9a95cfe61fd1/examples-shared/src/lib.rs#L16
pub async fn make_client(subs: Subscriptions) -> Result<Client, ()> {
    let (wheel, handler) = Wheel::new(Box::new(|err| {
        log::warn!("encountered an error {err:?}; shouldn't be fatal");
    }));

    let mut user = wheel.user();

    let discord = match Discord::new(DiscordApp::PlainId(APP_ID), subs, Box::new(handler)) {
        Ok(d) => d,
        Err(_) => {
            log::error!("unable to create discord client");
            Err(())?
        }
    };

    log::info!("waiting for handshake...");
    user.0.changed().await.unwrap();

    let user = match &*user.0.borrow() {
        UserState::Connected(user) => user.clone(),
        UserState::Disconnected(err) => {
            log::error!("failed to connect to Discord: {}", err);
            Err(())?
        }
    };
    
    #[cfg(debug_assertions)]
    log::info!("connected to Discord, local user is {:#?}", user);

    Ok(Client {
        discord,
        user,
        wheel,
    })
}
