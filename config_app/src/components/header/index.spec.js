import Header from './index.js';

describe("<Header>", () => {
  describe("#render", () => {
    let renderEl = (() => {
      return renderHTML(<Header />);
    });

    describe("header", () => {
      it("className is header", () => {
        expect(renderEl().querySelector("header").className).to.eq("header");
      });
    });

    describe("h1", () => {
      it("text is Configure Felux", () => {
        expect(renderEl().querySelector("h1").textContent).to.eq("Configure Felux");
      });
    });
  });
});
