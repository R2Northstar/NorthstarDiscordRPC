use parking_lot::Mutex;
use std::ffi::{c_char, CStr};

use crate::PLUGIN;

pub static JOIN_HANDLER_FUNCTION: Mutex<JoinHandler> = Mutex::new(default_join_handler);

type JoinHandler = extern "C" fn(*const c_char);

#[repr(C)]
#[must_use]
pub enum IniviteHandlerResult {
    Sucess,
    Failure,
}

#[repr(C)]
pub(crate) struct InviteHandler;

#[rrplug::as_interface]
impl InviteHandler {
    pub fn new() -> Self {
        Self
    }

    pub fn set_join_handler(&self, handler: JoinHandler) {
        *JOIN_HANDLER_FUNCTION.lock() = handler;
    }

    pub fn set_secret(&self, secret: *const c_char) -> IniviteHandlerResult {
        if secret.is_null() {
            return IniviteHandlerResult::Failure;
        }

        let Some(secret) = unsafe { CStr::from_ptr(secret) }.to_str().ok() else {
            return IniviteHandlerResult::Failure;
        };

        PLUGIN.wait().activity.lock().secrets.join = Some(secret.to_string());
        IniviteHandlerResult::Sucess
    }

    pub fn clear_secret(&self) {
        let secrets = &mut PLUGIN.wait().activity.lock().secrets;

        secrets.r#match = None;
        secrets.join = None;
        secrets.spectate = None;
    }
}

extern "C" fn default_join_handler(_secret: *const c_char) {}
