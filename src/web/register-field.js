class RegisterInputElement extends HTMLInputElement {
	/** @type {"low"|"high"|"full"} */
	position = "full"
	reg = "<unknown>"
	/** @type {"hex"|"decimal"|"binary"} */
	format = "decimal"
	validFormatSymbols = {
		hex: "0123456789abcdef",
		decimal: "0123456789",
		binary: "01"
	}

	onChange(e) {
		let parsed = parseInt(this.value, this.validFormatSymbols[this.format].length)
		if (parsed === NaN) {
			parsed = 0
		}

		this.setInputValue(parsed)
		this.render()
	}

	onKeyPress(e) {
		if (!this.validFormatSymbols[this.format].includes(e.key.toLocaleLowerCase())) {
			e.preventDefault()
		}
	}

	connectedCallback() {
		this.reg = this.getAttribute("reg").toLocaleLowerCase()
		this.position = this.getAttribute("position")
		this.classList.add("value")
		if (this.position !== "full") {
			this.classList.add("value--byte")
		}
		this.addEventListener("change", this.onChange)
		this.addEventListener("keypress", this.onKeyPress)
	}

	getInputValue() {
		const value = registers[this.reg].get()
		switch (this.position) {
		case "full": return value & 0xFFFF
		case "low":  return value & 0xFF
		case "high": return (value >> 8) & 0xFF
		}
	}

	setInputValue(value) {
		let current = registers[this.reg].get()
		let newValue
		switch (this.position) {
		case "full":
			newValue = value
			break;
		case "low":
			newValue = (current & 0xFF00) | (value & 0xFF)
			break;
		case "high":
			newValue = (current & 0x00FF) | ((value & 0xFF) << 8)
			break;
		}
		registers[this.reg].set(newValue)
	}

	render() {
		const value = this.getInputValue()
		if (this.format == "hex") {
			this.value = value.toString(16).padStart(this.position == "full" ? 4 : 2, "0")
		} else if (this.format == "decimal") {
			this.value = value
		} else if (this.format == "binary") {
			this.value = value.toString(2).padStart(this.position == "full" ? 16 : 8, "0")
		}
	}
}

class RegisterElement extends HTMLElement {
	reg = "<unknown>"
	readonly = false
	hasHighLow = false

	/** @type {RegisterInputElement} */
	valueElement = undefined
	/** @type {RegisterInputElement} */
	highByteElement = undefined
	/** @type {RegisterInputElement} */
	lowByteElement = undefined

	constructor() {
		super()
		this.readonly = this.getAttribute("readonly") !== null
		this.reg = this.getAttribute("reg").toLocaleLowerCase()
		this.hasHighLow = this.getAttribute("has-low-high") !== null

		const nameElement = this.appendChild(document.createElement("span"))
		nameElement.classList.add("name")
		nameElement.innerText = this.reg.toUpperCase()

		if (this.hasHighLow) {
			const highByteElem = document.createElement("input", { is: "register-input" })
			highByteElem.setAttribute("reg", this.reg)
			highByteElem.setAttribute("position", "high")
			if (this.readonly) highByteElem.setAttribute("readonly", "")
			this.highByteElement = this.appendChild(highByteElem)

			const lowByteElem = document.createElement("input", { is: "register-input" })
			lowByteElem.setAttribute("reg", this.reg)
			lowByteElem.setAttribute("position", "low")
			if (this.readonly) lowByteElem.setAttribute("readonly", "")
			this.lowByteElement = this.appendChild(lowByteElem)
		} else {
			const elem = document.createElement("input", { is: "register-input" })
			elem.setAttribute("reg", this.reg)
			elem.setAttribute("position", "full")
			if (this.readonly) elem.setAttribute("readonly", "")
			this.valueElement = this.appendChild(elem)
		}
	}

	render() {
		if (this.hasHighLow) {
			this.lowByteElement.render()
			this.highByteElement.render()
		} else {
			this.valueElement.render()
		}
	}
}

customElements.define("register-input", RegisterInputElement, { extends: "input" })
customElements.define("register-field", RegisterElement)
