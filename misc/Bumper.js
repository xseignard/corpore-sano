const EventEmitter = require('events')

class Bumper extends EventEmitter {
  /**
   * Constructor
   * @param {Number} boardId - id of the board, used to compute IP
   * @param {Number} bumperId - id on the board of the bumper
   * @param {Object} udpClient - instance of udp client
   * @param {Number} holdTime - time (in ms) to trigger a 'hold' event, defaults to 3000ms
   */
  constructor(boardId, bumperId, udpClient, holdTime = 3000) {
    super()
    this.boardId = boardId
    this.bumperId = bumperId
    this.host = `2.0.0.${this.boardId}`
    this.port = 8888
    this.client = udpClient
    this.lastPress = new Date()
    this.lastRelease = new Date()
    this.holdTime = holdTime
    this.canSendHold = false
    setInterval(() => {
      const now = new Date()
      if (
        this.lastPress > this.lastRelease &&
        now - this.lastPress > this.holdTime &&
        this.canSendHold
      ) {
        this.emit('hold')
        this.canSendHold = false
      }
    }, 50)
    // handle incoming messages
    this.client.on('message', (data, info) => {
      const address = info.address
      // only check messages coming from the board IP address
      if (address === this.host) {
        const msg = data.toString()
        // boolean to check whether this bumper is concerned by the message
        const messageContainsBumperId = msg
          .substr(1) // remove leading P or R
          .split('#') // split into array of bumpers
          .map(e => parseInt(e, 10)) // parse ints
          .includes(this.bumperId) // return true if the array contains the bumper id

        // pressed message
        if (msg.startsWith('P') && messageContainsBumperId) {
          this.lastPress = new Date()
          this.canSendHold = true
          this.emit('press')
        }
        // released message
        if (msg.startsWith('R') && messageContainsBumperId) {
          this.lastRelease = new Date()
          this.canSendHold = true
          this.emit('release')
        }
      }
    })
  }

  /**
   * Turn on/off the buzzer
   * @param {Boolean} onOff - wether to turn on or off the buzzer
   */
  buzz(onOff) {
    this.client.send(
      // create a message like the following: 0B1
      Buffer.from(`${this.bumperId}B${onOff ? 1 : 0}`),
      this.port,
      this.host,
      err => {
        if (err) this.emit('error', err)
      }
    )
  }

  /**
   * Send RGB color
   * @param {Number} r - red amount (0 -> 65535)
   * @param {Number} g - green amount (0 -> 65535)
   * @param {Number} b - blue amount (0 -> 65535)
   */
  rgb(r, g, b) {
    const paddedR = `${r}`.padStart(5, '0')
    const paddedG = `${g}`.padStart(5, '0')
    const paddedB = `${b}`.padStart(5, '0')
    this.client.send(
      // create a message like the following: 0C65535;65535;65535
      Buffer.from(`${this.bumperId}C${paddedR};${paddedG};${paddedB}`),
      this.port,
      this.host,
      err => {
        if (err) this.emit('error', err)
      }
    )
  }

  /**
   * Set RGB led to red
   */
  red() {
    this.rgb(65535, 0, 0)
  }

  /**
   * Set RGB led to green
   */
  green() {
    this.rgb(0, 65535, 0)
  }

  /**
   * Set RGB led to blue
   */
  blue() {
    this.rgb(0, 0, 65535)
  }

  /**
   * Set RGB led to white
   */
  white() {
    this.rgb(65535, 65535, 65535)
  }

  /**
   * Set RGB led to black (aka. turn it off)
   */
  black() {
    this.rgb(0, 0, 0)
  }

  /**
   * Set debounce time
   * @param {Number} debounceTime - debounce time in ms (0 -> 999)
   */
  setDebounceTime(debounceTime) {
    const paddedDebounceTime = `${debounceTime}`.padStart(5, '0')
    this.client.send(
      // create a message like the following: 0D100
      Buffer.from(`${this.bumperId}D${paddedDebounceTime}`),
      this.port,
      this.host,
      err => {
        if (err) this.emit('error', err)
      }
    )
  }

  /**
   * Set hold time
   * @param {Number} holdTime - hold time in ms
   */
  setHoldTime(holdTime) {
    this.holdTime = holdTime
  }
  /**
   * Compute an id for the bumper
   * @returns the computed id
   */
  getId() {
    return this.boardId * 10 + this.bumperId
  }
}

module.exports = Bumper
