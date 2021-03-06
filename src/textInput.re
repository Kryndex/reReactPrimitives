type state = {
  height: option int,
  inputRef: ref (option Dom.element),
  focused: bool
};

type action =
  | Change string
  | Focus
  | Blur
  | SetHeight int;

let component = ReasonReact.reducerComponent "TextInput";

external getStyle : DomRe.Element.t => Dom.cssStyleDeclaration = "style" [@@bs.get];

let setInputRef inputRef {ReasonReact.state: state} => state.inputRef := Js.Null.to_opt inputRef;

/* TODO: manage types and local validation */
let make
    ::multiline=false
    ::autoSize=false
    ::autoFocus=false
    ::value
    ::style=?
    ::onTextChange
    ::onKeyDown=?
    ::onPaste=?
    ::onFocus=?
    ::onBlur=?
    ::focusedStyle=?
    ::placeholder=""
    ::rows=1
    _children => {
  let measureAndSetHeight () {ReasonReact.state: state, ReasonReact.reduce: reduce} =>
    switch state.inputRef {
    | {contents: Some element} =>
      CssStyleDeclarationRe.setProperty "height" "0" "" (getStyle element);
      let height = DomRe.Element.scrollHeight element;
      CssStyleDeclarationRe.setProperty
        "height" (string_of_int height ^ "px") "" (getStyle element);
      reduce (fun height => SetHeight height) height
    | _ => ()
    };
  let handleResize {ReasonReact.handle: handle} =>
    if (multiline && autoSize) {
      Bs_webapi.requestAnimationFrame (fun _ => handle measureAndSetHeight ())
    };
  let handleChange value self => {
    onTextChange value;
    handleResize self
  };
  {
    ...component,
    initialState: fun () => {height: None, inputRef: ref None, focused: false},
    didMount: fun self => {
      handleResize self;
      ReasonReact.NoUpdate
    },
    reducer: fun action state =>
      switch action {
      | Focus => ReasonReact.Update {...state, focused: true}
      | Blur => ReasonReact.Update {...state, focused: false}
      | Change value => ReasonReact.SideEffects (handleChange value)
      | SetHeight height => ReasonReact.Update {...state, height: Some height}
      },
    render: fun {reduce, state, handle} => {
      let sizingStyle =
        ReactDOMRe.Style.make
          resize::"none"
          boxSizing::"content-box"
          fontSize::"16px"
          height::(
            switch state.height {
            | None => "auto"
            | Some height => string_of_int height ^ "px"
            }
          )
          ();
      ReactDOMRe.createElement
        (multiline ? "textarea" : "input")
        props::(
          ReactDOMRe.props
            ref::(handle setInputRef)
            ::rows
            style::(
              ReactDOMRe.Style.combine
                (
                  switch style {
                  | Some style => ReactDOMRe.Style.combine style sizingStyle
                  | None => sizingStyle
                  }
                )
                (
                  switch focusedStyle {
                  | Some style when state.focused == true => style
                  | _ => ReactDOMRe.Style.make ()
                  }
                )
            )
            onChange::(
              reduce (
                fun event =>
                  Change (ReactDOMRe.domElementToObj (ReactEventRe.Form.target event))##value
              )
            )
            ::?onKeyDown
            ::?onPaste
            onFocus::(
              fun event => {
                switch onFocus {
                | Some onFocus => onFocus event
                | None => ()
                };
                reduce (fun () => Focus) ()
              }
            )
            onBlur::(
              fun event => {
                switch onBlur {
                | Some onBlur => onBlur event
                | None => ()
                };
                reduce (fun () => Blur) ()
              }
            )
            ::value
            ::placeholder
            autoFocus::(Js.Boolean.to_js_boolean autoFocus)
            ()
        )
        [||]
    }
  }
};
