import {useState, useEffect, useRef} from 'react'
function App() {

  const [buttonText, setButtonText] = useState("Begin Brew")
  const [isBrewActive, setIsBrewActive] = useState(false)

  const ws = useRef(null);

  useEffect(() => {

      ws.current = new WebSocket(`wss://nc2mnffrj0.execute-api.us-west-1.amazonaws.com/production/`)
      
      ws.current.onmessage = (event)=>{

          console.log(event)

          // if (event.data !== ''){
          //     const newMessage =  JSON.parse(JSON.parse(event.data));
          //     console.log(newMessage)
            
          // }   
          
      }

      ws.current.onclose = (event)=>{
          console.log("closed!")
      }
      ws.current.onopen = (event) =>{
        //on open send a ping/pong message to the arduino
          console.log("opened!")
      }

      ws.current.onerror = (event) =>{
        setButtonText("Begin Brew")
      }

      return () => {
          ws.current.close();
      }

  }, [])

  function sendBrewMessage(){
    
    ws.current.send(JSON.stringify({action: "message", message: !isBrewActive ? "start" : "stop"}))

    setIsBrewActive(true)

    setButtonText("Stop brew")
  }
    
  return (

    <div>
      <button onClick={sendBrewMessage}>{buttonText}</button>
    </div>
  );
}

export default App;
