use rrplug::{bindings::squirreldatatypes::SQObjectType, prelude::*};

#[allow(clippy::missing_safety_doc, unused)]
pub unsafe fn find_var_on_stack(sqvm: *mut HSquirrelVM, var_type: SQObjectType) {
    let sqvm = &*sqvm;
    (0..10)
        .map(|i| (sqvm._stackOfCurrentFunction.add(i), i))
        .map(|(ptr, i)| (&*ptr, i))
        // .filter(|(object, _)| object._Type == var_type)
        // .map(|(_, e)| e)
        // .next()
        // .unwrap_or(1) as i32
        .for_each(|(obj, e)| log::info!("found var at {e} {:?}", obj._Type))
}
