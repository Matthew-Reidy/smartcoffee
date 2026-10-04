//
//  websocket.swift
//  coffee-app
import Foundation
import Observation

@Observable
class Websocket {
    var isConnected = false
    var isBrewActive = false

    private var webSocketTask: URLSessionWebSocketTask?

    func connect() {
        guard let url = URL(string: "wss://nc2mnffrj0.execute-api.us-west-1.amazonaws.com/production/") else {
            print("Invalid URL")
            return
        }
        let session = URLSession(configuration: .default)
        webSocketTask = session.webSocketTask(with: url)
        webSocketTask?.resume()
        isConnected = true
        print("WebSocket connected")
        receiveMessage()
    }

    func sendBrew(active: Bool) {
        let payload: [String: String] = ["action": "brew", "state": active ? "on" : "off"]
        guard let data = try? JSONSerialization.data(withJSONObject: payload),
              let json = String(data: data, encoding: .utf8) else {
            print("Failed to encode brew command")
            return
        }
        webSocketTask?.send(.string(json)) { error in
            if let error {
                print("WebSocket send error: \(error)")
            } else {
                print("Sent brew command: \(active ? "on" : "off")")
            }
        }
    }

    func receiveMessage() {
        webSocketTask?.receive { [weak self] result in
            guard let self else { return }
            switch result {
            case .success(let message):
                let text: String?
                switch message {
                case .string(let s): text = s
                case .data(let d):   text = String(data: d, encoding: .utf8)
                @unknown default:    text = nil
                }
                if let text { self.handleMessage(text) }
                self.receiveMessage()
            case .failure(let error):
                print("WebSocket receive error: \(error)")
                DispatchQueue.main.async { self.isConnected = false }
            }
        }
    }

    private func handleMessage(_ text: String) {
        print("Received: \(text)")
        guard let data = text.data(using: .utf8),
              let json = try? JSONSerialization.jsonObject(with: data) as? [String: Any] else { return }
        if let state = json["state"] as? String {
            DispatchQueue.main.async { self.isBrewActive = state == "on" }
        }
    }

    func disconnect() {
        webSocketTask?.cancel(with: .goingAway, reason: nil)
        isConnected = false
        isBrewActive = false
    }
}
