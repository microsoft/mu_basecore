use goblin::{error, Object};
use std::path::Path;
use std::env;
use std::fs;

fn run () -> error::Result<()> {
    for (i, arg) in env::args().enumerate() {
        if i == 1 {
            let path = Path::new(arg.as_str());
            let buffer = fs::read(path)?;
            if let Object::PE(pe) = Object::parse(&buffer)? {
                println!("pe: {:#?}", &pe);
            }
        }
    }
    Ok(())
}

fn main() {
    let result = run();
}
