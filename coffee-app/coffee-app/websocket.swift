//
//  websocket.swift
//  coffee-app
import Foundation
import Observation

@Observable
class Websocket{

    private var webSocketTask: URLSessionWebSocketTask?
    
    func connect(){
        
        guard let url = URL(string: "wss://nc2mnffrj0.execute-api.us-west-1.amazonaws.com/production/") else {
                print("Invalid URL")
                return
        }
        
        let session = URLSession(configuration: .default)
        webSocketTask = session.webSocketTask(with: url)
        webSocketTask?.resume()
        print("WebSocket Connected")
    }
    
    func sendMessage(message: String){
        webSocketTask?.send(URLSessionWebSocketTask.Message.string(message)){
            error in
            if let error = error {
                print("WebSocket sending error: \(error)")
            } else {
                print("Message sent: \(message)")
            }
        }
    }
    
//    func recieveMessage(){
//        
//        let webSocketTask?.receive{
//            result in
//            if(result == .success){
//                
//            }else{
//                let error
//                
//            }
//        }
//    }
    
    func disconnect(){
        
        webSocketTask?.cancel(with: .goingAway, reason: nil)
    }
    
}
